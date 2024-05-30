/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSession.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 23:54:28
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivSession.h"
#include "PrivEventManager.h"
#include "infra/include/Timestamp.h"
#include "infra/include/Logger.h"

PrivSession::PrivSession(IPrivSessionManager *manager, const char* name, uint32_t sessionId, RPCServer *rpc_server):
    PrivSessionBase(nullptr, name, sessionId, rpc_server), mSessionManager(manager), mStreamAlive(false), mSmartEventTimer("test") {
    std::lock_guard<std::mutex> guard(mSubscribedEventsMtx);
    mSubscribedEvents.clear();
}

PrivSession::~PrivSession() {
    tracef("PrivSession::~PrivSession %p\n", this);
    std::lock_guard<std::mutex> guard(mSubscribedEventsMtx);
    mSubscribedEvents.clear();
    closeSession();
}

bool PrivSession::initial(std::shared_ptr<infra::TcpSocket>& newSock, int32_t bufferLen, bool bRecv) {
    if (!PrivSessionBase::initial(newSock, bufferLen, bRecv)) {
        return false;
    }
    initMethodList();
    return true;
}

bool PrivSession::initial(std::shared_ptr<infra::TcpSocket>& newSock, const char *buffer, int32_t len) {
    if (!PrivSessionBase::initial(newSock, buffer, len)) {
        return false;
    }
    initMethodList();
    return true;
}

#define REGISTER_METHOND_FUNC(x) registerMethodFunc(#x, &PrivSession::x, this)
void PrivSession::initMethodList() {
    REGISTER_METHOND_FUNC(login);
    REGISTER_METHOND_FUNC(start_preview);
    REGISTER_METHOND_FUNC(stop_preview);
}

bool PrivSession::call(std::string key, MessagePtr &message) {
    auto it = map_invokers_.find(key);
    if (it == map_invokers_.end()) {
        message->code = 1;
        message->message = "not support this method";
        errorf("not support method:%s\n", message->method.c_str());
    } else {
        return it->second(message);
    }
    return false;
}

void PrivSession::onRequest(MessagePtr &msg) {
    infof("request method %s\n", msg->method.c_str());
    bool result = call(msg->method, msg);
    if (result) {
        msg->code = 0;
        msg->message = "ok";
    } 
    this->response(msg);  ///应答
}

bool PrivSession::login(MessagePtr &msg) {
    return true;
}

void PrivSession::onMediaData(MessagePtr &msg, const char *buffer, uint32_t size) {
    tracef("on media data len:%d\n", size);
}

PrivSubSessionPtr PrivSession::addNewSubSession(int32_t channel, int32_t sub_channel) {
    tracef("add new sub session channel:%d sub_channel:%d\n", channel, sub_channel);
    ///精确到毫秒
    uint32_t id = uint32_t(infra::getCurrentTimeUs());
    ///最高4位用来做别的定义
    id &= 0x0FFFFFFF;

    SessionPair index(channel, sub_channel);
    PrivSubSessionPtr sub(new PrivSubSession(this, "preview", id));
    if (sub == nullptr) {
        errorf("new PrivSubSession error\n");
        return nullptr;
    }

    std::lock_guard<std::mutex> guard(mSessionMapMtx);
    mSubSessionMap[index] = sub;
    return sub;
}

PrivSubSessionPtr PrivSession::getSubSession(int32_t channel, int32_t sub_channel) {
    SessionPair index(channel, sub_channel);
    std::lock_guard<std::mutex> guard(mSessionMapMtx);
    auto it = mSubSessionMap.find(index);
    if (it != mSubSessionMap.end()) {
        return it->second;
    }
    return PrivSubSessionPtr();
}

bool PrivSession::getChannelStreamType(MessagePtr &msg, int32_t &channel, int32_t &stream) {
    if (msg->data.isMember("channelNo") && msg->data["channelNo"].isInt()) {
        channel = msg->data["channelNo"].asInt();
    } else {
        msg->code = 1001;msg->message = "param channelNo error";
        return false;
    }
    if (msg->data.isMember("streamType") && msg->data["streamType"].isInt()) {
        channel = msg->data["streamType"].asInt();
    } else {
        msg->code = 1001;
        msg->message = "param streamType error";
        return false;
    }
    return true;
}

bool PrivSession::start_preview(MessagePtr &msg){
    int32_t channel = 1,stream = 1;
    if (msg->data.isMember("channelNo") && msg->data["channelNo"].isInt()){
        channel = msg->data["channelNo"].asInt();
    } else {
        msg->code = 1001;msg->message = "param channelNo error";
        return false;
    }

    if (msg->data.isMember("streamType") && msg->data["streamType"].isInt()){
        stream = msg->data["streamType"].asInt();
    } else {
        msg->code = 1001;
        msg->message = "param streamType error";
        return false;
    }

    PrivSubSessionPtr sub = getSubSession(channel, stream);
    if (sub.get() == nullptr) {
        ///创建子会话，用做发流
        tracef("create sub session\n");
        sub = addNewSubSession(channel, stream);
    }

    sub->setCallback(PrivSubSession::CallbackSignal::Proc(&PrivSession::subSessionSendFailed, this));

    if (sub->streaming()) {
        msg->sessionId = sub->getSessionId();
        Json::Value data;
        data["sessionId"] = msg->sessionId;
        msg->data = data;
        msg->code = 101;
        msg->message = "stream had started!";
        return false;
    }

    if (!sub->startStream(channel, stream)){
        errorf("subSession startStream failed\n");
        return false;
    }

    msg->sessionId = sub->getSessionId();
    Json::Value data = Json::nullValue;
    data["sessionId"] = msg->sessionId;
    msg->data = data;

    ///发流状态
    mStreamAlive = true;
    return true;
}

bool PrivSession::stop_preview(MessagePtr &msg) {
    int32_t channel = 1,streamType = 1;
    if (msg->data.isMember("channelNo") && msg->data["channelNo"].isInt()){
        channel = msg->data["channelNo"].asInt();
    } else{
        msg->code = 1001;msg->message = "param channelNo error";
        return false;
    }

    if (msg->data.isMember("streamType") && msg->data["streamType"].isInt()){
        channel = msg->data["streamType"].asInt();
    } else {
        msg->code = 1001;msg->message = "param streamType error";
        return false;
    }

    PrivSubSessionPtr sub = getSubSession(channel, streamType);
    if (!sub->stopStream(channel, streamType)){
        msg->code = 1001;msg->message = "stop preview error";
        return false;
    }

    mStreamAlive = false;
    return true;
}

bool PrivSession::dealVideoCapability(MessagePtr &msg){

    int32_t channel = 1,streamType = 1;
    if (!getChannelStreamType(msg, channel, streamType)){
        //暂时注释掉
    //    return false;
    }

    Json::Value data = Json::nullValue;
    getVideoCapability(channel, streamType, data);

    ///todo 调用底层借口获取能力集

    /*Json::Value data;
    data["channelNo"] = channel;
    data["streamType"] = streamType;
    Json::Value compression;
    Json::Value H264;
    H264["name"] = "H264";
    H264["value"] = 1;
    H264["profile"][0]["name"] = "base";
    H264["profile"][0]["value"] = 1;
    H264["profile"][1]["name"] = "main";
    H264["profile"][1]["value"] = 2;
    H264["profile"][2]["name"] = "extend";
    H264["profile"][2]["value"] = 3;
    H264["profile"][3]["name"] = "high";
    H264["profile"][3]["value"] = 4;
    H264["gop"]["min"] = 1;
    H264["gop"]["max"] = 200;
    H264["fps"]["minNum"] = 1;
    H264["fps"]["minDen"] = 4;
    H264["fps"]["max"] = 25;    
    H264["bitrate"]["min"] = 32;
    H264["bitrate"]["max"] = 4096;
    H264["bitrateControl"][0]["name"] = "定码率";
    H264["bitrateControl"][0]["value"] = 0;
    H264["bitrateControl"][1]["name"] = "变码率";
    H264["bitrateControl"][1]["value"] = 1;
    H264["imageSize"][0]["name"] = "D1(704*576)";
    H264["imageSize"][0]["value"] = 0;
    H264["imageSize"][1]["name"] = "VGA(640*780)";
    H264["imageSize"][1]["value"] = 1;
    H264["imageSize"][2]["name"] = "720P(1280*720)";
    H264["imageSize"][2]["value"] = 4;
    H264["imageSize"][3]["name"] = "1080P(1920*1080)";
    H264["imageSize"][3]["value"] = 5;
    H264["smartEncode"] = false;
    H264["streamSmooth"]["min"] = 0;
    H264["streamSmooth"]["max"] = 100;    
    H264["imageQuality"]["min"] = 1;
    H264["imageQuality"]["max"] = 10; 
    compression[0] = H264;
    data["compression"] = compression;
    */
    msg->data = data;
    return true;
}

bool PrivSession::dealVideoConfig(MessagePtr &msg){

    int32_t channel = 0,stream = 0;
    if (!getChannelStreamType(msg, channel, stream)){
        return false;
    }

    Json::Value data;
    data["channelNo"]    = channel;
    data["streamType"]   = stream;
    data["videoType"]    = 1;
    data["compression"]  = 1;
    data["smartEncode"]  = false;
    data["imageWidth"]   = 720;
    data["imageHeight"]  = 1712;
    data["fps"]          = 25.0;
    data["bitrate"]      = 1024;
    data["profile"]      = 1;
    data["gop"]          = 50;
    data["streamSmooth"] = 50;
    msg->data = data;
    return true;
}

bool PrivSession::startTalkBack(MessagePtr &msg){
    Json::Value data = Json::nullValue;
    msg->data = data;
    return true;
}

bool PrivSession::stopTalkBack(MessagePtr &msg){
    Json::Value data = Json::nullValue;
    msg->data = data;
    return true;
}

//订阅智能事件，当前直接全部订阅
bool PrivSession::subscribeSmartEvent(MessagePtr &msg) {
    if (!msg->data.isMember("event") || !msg->data["event"].isArray()) {
        Json::Value data = Json::nullValue;
        msg->data = data;
        msg->code = 1001;
        msg->message = "paramter event error";
        return false;
    }

    for (auto &event : msg->data["event"]) {
        if (!event.isString()) {
            msg->code = 1001;
            msg->message = "event parmeter is not string";
            return false;
        }
        std::string eName = event.asString();

        std::lock_guard<std::mutex> guard(mSubscribedEventsMtx);
        std::vector<std::string>::iterator it = std::find(mSubscribedEvents.begin(), mSubscribedEvents.end(), eName);
        if (it != mSubscribedEvents.end()) {
            msg->code = 1001;
            std::string m = std::string("event ") + eName + " had subscribe";
            msg->message = m;
            return false;
        }

        if (PrivEventManager::instance()->canSubscribeEvent(eName.c_str())) {
            mSubscribedEvents.push_back(eName.c_str());
        } else {
            msg->code = 1001;
            std::string m = std::string("event ") + eName + " cannot subscribe";
            msg->message = m;
            return false; 
        }
    }
    Json::Value data = Json::nullValue;
    msg->data = data;
    //only for test
    //mSmartEventTimer.start(UBase::CTimer::Proc(&PrivSession::sendSmartEventProc, this), 2*1000, 2*1000);
    return true;    
}

void PrivSession::sendSmartEventProc(uint64_t arg){
    /*static int32_t flow = 0;
    static int32_t voice = 0;
    UBase::CTime time = UBase::CTime::getCurrentTime();
    char timeBuf[64];
    time.format(timeBuf);
    Json::Value data = Json::nullValue;
    int32_t location = 5;
    data["method"] = "smartEvent";

    data["data"]["passengaerFlow"]["population"] = flow * 8;
    data["data"]["passengaerFlow"]["crowded"] = ++flow > 3 ? flow=0 : flow;
    data["data"]["passengaerFlow"]["time"] = timeBuf;
    data["data"]["passengaerFlow"]["location"] = location;

    data["data"]["behavior"]["action"] = 0;
    data["data"]["behavior"]["time"] = timeBuf;
    data["data"]["behavior"]["location"] = location;

    data["data"]["leave"]["people"]["count"] = flow * 2;
    data["data"]["leave"]["things"]["count"] = 1;
    data["data"]["leave"]["time"] = timeBuf;
    data["data"]["leave"]["location"] = location;  //车厢位置

    data["data"]["voice"]["event"] = ++voice > 2 ? voice=0 : voice;
    data["data"]["voice"]["decibel"] = voice;
    data["data"]["voice"]["direction"] = voice*2;
    data["data"]["voice"]["time"] = timeBuf;  //车厢位置
    data["data"]["voice"]["location"] = location;  //车厢位置
    if (this->sendRequest(data) < 0) {
        errorf("sendRequest error\n");
    }
    */
}

void PrivSession::getVideoCapability(int32_t channel, int32_t streamType, Json::Value &config) {
    /*config["channelNo"] = channel;
    config["streamType"] = streamType;
    Json::Value compression = Json::nullValue;
    Json::Value H264;
    H264["name"] = "H264";
    H264["value"] = 1;
    H264["profile"][0]["name"] = "base";
    H264["profile"][0]["value"] = 1;
    H264["profile"][1]["name"] = "main";
    H264["profile"][1]["value"] = 2;
    H264["profile"][2]["name"] = "extend";
    H264["profile"][2]["value"] = 3;
    H264["profile"][3]["name"] = "high";
    H264["profile"][3]["value"] = 4;
    H264["gop"]["min"] = 1;
    H264["gop"]["max"] = 200;
    H264["fps"]["minNum"] = 1;
    H264["fps"]["minDen"] = 4;
    H264["fps"]["max"] = 25;    
    H264["bitrate"]["min"] = 32;
    H264["bitrate"]["max"] = 4096;
    H264["bitrateControl"][0]["name"] = "定码率";
    H264["bitrateControl"][0]["value"] = 0;
    H264["bitrateControl"][1]["name"] = "变码率";
    H264["bitrateControl"][1]["value"] = 1;
    H264["imageSize"][0]["name"] = "D1(704*576)";
    H264["imageSize"][0]["value"] = 0;
    H264["imageSize"][1]["name"] = "VGA(640*780)";
    H264["imageSize"][1]["value"] = 1;
    H264["imageSize"][2]["name"] = "720P(1280*720)";
    H264["imageSize"][2]["value"] = 4;
    H264["imageSize"][3]["name"] = "1080P(1920*1080)";
    H264["imageSize"][3]["value"] = 5;
    H264["smartEncode"] = false;
    H264["streamSmooth"]["min"] = 0;
    H264["streamSmooth"]["max"] = 100;    
    H264["imageQuality"]["min"] = 1;
    H264["imageQuality"]["max"] = 10; 
    compression[0] = H264;
    config["compression"] = compression;
    */
}

bool PrivSession::streamStatus(){
    return mStreamAlive;
}

bool PrivSession::sendEvent(const char* name, const Json::Value &event) {
    std::lock_guard<std::mutex> guard(mSubscribedEventsMtx);
    std::vector<std::string>::iterator it = std::find(mSubscribedEvents.begin(), mSubscribedEvents.end(), name); 
    if (it != mSubscribedEvents.end()) {
        Json::Value data = Json::nullValue;
        data["method"] = "smartEvent";
        data["data"] = event;
        if (this->sendRequest(data) < 0) {
            errorf("sendRequest error\n");
        }
    } else {
        //debugf("event:%s not subscribe\n", name);
    }  
    return true;
}

void PrivSession::subSessionSendFailed(std::string error) {
    warnf("mSessionManager collect\n");
    mSessionManager->remove(getSessionId());
}

void PrivSession::closeSession() {
    ///关闭流的发送
    std::lock_guard<std::mutex> guard(mSessionMapMtx);
    for (const auto& it : mSubSessionMap){
        it.second->stopStream();
    }

    ///删除所有子会话
    mSubSessionMap.clear();
    //close();
}
