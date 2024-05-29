/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSessionBase.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 10:10:27
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "jsoncpp/include/json.h"
#include "PrivSessionBase.h"
#include "PrivSessionManager.h"
#include "infra/include/Logger.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/Utils.h"

///主要是接收信令，buffer可以开小一点
#define SessionBaseBufferSize (16*1024)
PrivSessionBase::PrivSessionBase(PrivSessionBase *master,const char* name, uint32_t sessionId, RPCServer *rpc_server):
    mBuffer(nullptr), mBufferLen(0), mBufferDataLen(0), mName(name), 
    mSessionId(sessionId), mMaster(master), mProxy(nullptr), mSock(nullptr),
    rpc_server_(rpc_server) {
}

PrivSessionBase::~PrivSessionBase() {
    tracef("~PrivSessionBase\n");
    if (mBuffer) {
        delete mBuffer;
        mBuffer = nullptr;
    }
}

bool PrivSessionBase::initial(std::shared_ptr<infra::TcpSocket> & newsock, int32_t bufferLen, bool bRecv) {
    mSock = newsock;
    mBRecv = bRecv;
    if (!mBRecv) {
        mBufferLen = 0;
        mBuffer = nullptr;
        return true;
    }

    mBufferLen = bufferLen;
    mBuffer = new char[bufferLen];
    if (mBuffer == nullptr) {
        errorf("private base session new buffer %d error\n", mBufferLen);
        mBufferLen = 0;
        return false;
    }

    memset(mBuffer, 0x00, mBufferLen);

    setSendBufferLen(1024 * 1024);

    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(newsock->getHandle(), event_type, shared_from_this())) {
        errorf("initial error\n");
        return false;
    }

    return true;
}

bool PrivSessionBase::initial(std::shared_ptr<infra::TcpSocket>& newsock, const char *buffer, int32_t len) {
    if (buffer == nullptr) {
        errorf("initial error\n");
        return false;
    }

    mBufferLen = SessionBaseBufferSize;
    mBuffer = new char[mBufferLen];
    if (mBuffer == nullptr) {
        errorf("private base session new buffer %d error\n", mBufferLen);
        mBufferLen = 0;
        return false;
    }

    memset(mBuffer, 0x00, mBufferLen);
    memcpy(mBuffer, buffer, len);
    mBufferDataLen = len;

    mSock = newsock;
    setSendBufferLen(1024 * 1024);

    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(newsock->getHandle(), event_type, shared_from_this())) {
        errorf("initial error\n");
        return false;
    }

    MessagePtr msg = parse();
    if (!msg) {
        return false;
    }

    process(msg);
    return true;
}

int32_t PrivSessionBase::setSendBufferLen(int32_t length) {
    mSock->setSendBuffer(length);
    return 0;
}

void PrivSessionBase::setProxy(IPrivSessionProxy *proxy) {
    mProxy = proxy;
}

IPrivSessionProxy* PrivSessionBase::getProxy(){
    return mProxy;
}

int32_t PrivSessionBase::onRead(int32_t socketFd) {
    if (socketFd != mSock->getHandle()) {
        errorf("socketFd(%d) != getHandle(%d)\n", socketFd, mSock->getHandle());
        return -1;
    }

    int32_t ret = -1;
    char *recvBuffer = mBuffer + mBufferDataLen;
    int32_t recvBufferLen = mBufferLen - mBufferDataLen;

    if (mProxy) {
        ret = mProxy->recvData(mSock, recvBuffer, recvBufferLen);
    } else {
        ret = mSock->recv(recvBuffer, recvBufferLen);
    }

    if (ret > 0) {
        tracef("recv data len:%d............\n", ret);
        #if 0
        unsigned char *p = (unsigned char*)recvBuffer;
        for (auto i = 0; i < (ret > 32 ? 32 : ret); i++){
            printf("%02x ", p[i]);
        }
        printf("\n");
        #endif

        mBufferDataLen += ret;
        MessagePtr msg;
        do {
            msg = parse();
            if (msg.get()){
                process(msg);
            }
        } while(msg.get() && mBufferDataLen > 0);

    } else if (ret < 0){
        warnf("socket disconnect\n");
        onExitSession();
        return -1;
    }

    return 0;
}

int32_t PrivSessionBase::onException(int32_t socketFd) {
    if (socketFd != mSock->getHandle()) {
        return -1;
    }
    errorf("socket fd %d exception!\n", socketFd);
    onExitSession();
    return -1;
}

void PrivSessionBase::onExitSession() {
    warnf("to remove fd:%d\n", mSock->getHandle());
    if (!infra::NetworkThreadPool::instance()->delSocketEvent(mSock->getHandle(), shared_from_this())) {
        errorf("delSocketEvent error\n");
    }  
    onDisconnect();
}

bool PrivSessionBase::response(MessagePtr &msg) {
    Json::Value root;
    root["code"] = msg->code;
    root["message"] = msg->message;
    root["method"]  = msg->method;

    if (!msg->data.empty()){
        root["data"] = msg->data;
    }

    std::string body = root.toStyledString();

    auto *head = reinterpret_cast<PrivateDataHead*>(mCmdSendBuffer);
    head->tag[0] = '@';
    head->tag[1] = '@';
    head->tag[2] = '@';
    head->tag[3] = '@';
    head->version = 0x10;
    head->type = 0;
    if (msg->method == "keepAlive"){
        head->type = 2;  ///心跳类型
    }

    head->flag = 0x80;
    head->sequence   = infra::htonl(msg->sequence);   ///应答透传sequence
    head->sessionId  = infra::htonl(msg->sessionId);
    head->bodyLen    = infra::htonl((uint32_t)body.size());
    head->encrypt = 0;

    int32_t copyLen = body.size() < (sizeof(mCmdSendBuffer) - sizeof(PrivateDataHead)) ? (int32_t)body.size() : (sizeof(mCmdSendBuffer) - sizeof(PrivateDataHead));
    memcpy(head->buf, body.c_str(), copyLen);

    tracef("to response len:%d\n%s", body.size(), body.c_str());

    if (mProxy) {
        return mProxy->sendData((const char*)head, copyLen + sizeof(PrivateDataHead));
    }

    return send((const char*)head, copyLen + sizeof(PrivateDataHead)) >= 0;
}

int32_t PrivSessionBase::send(const char* buffer, int32_t len) {
    return mSock->send(buffer, len);
}

int32_t PrivSessionBase::sendCommand(const char* data, int32_t dataLen){
    int32_t ret = send(data, dataLen);
    if (ret < 0) {
        errorf("PrivSessionBase::sendCommand failed ret:%d\n", ret);
    }
    return ret;
}

///解析协议
MessagePtr PrivSessionBase::parse() {
    int32_t frameLen = 0;
    MessagePtr msg = parse(mBuffer, mBufferDataLen, frameLen);
    if (msg.get() == nullptr) {
        return msg;
    }

    int32_t leftLen = mBufferDataLen - frameLen;
    if (leftLen > 0) {
        memcpy(mBuffer, mBuffer + frameLen, leftLen);
        mBufferDataLen = leftLen;
    } else {
        mBufferDataLen = 0;
    }
    tracef("after parse mBufferDataLen:%d\n", mBufferDataLen);
    return msg;
}

MessagePtr PrivSessionBase::parse(const char* buffer, int32_t len, int32_t &usedLen){
    usedLen = 0;
    if (buffer == nullptr || len < sizeof(PrivateDataHead)){
        errorf("buffer is nullptr or len(%d) is too small\n", len);
        return nullptr;
    }

    ///查找消息头@@@@
    int32_t i = 0U;
    for (; i < len - 3; i++) {
        if (buffer[i+0] == '@' && buffer[i+1] == '@' && buffer[i+2] == '@' && buffer[i+3] == '@') {
            break;
        }
    }

    if (i >= (len - 3)) {
        errorf("not found head tag '@@@@' \n");
        usedLen = len;   ///消耗掉所有数据
        return nullptr;
    }

    if (i > 0) {
        warnf("head tag '@@@@' start with %d\n", i);
        usedLen += i;
    }

    char *pBuffer = (char*)&buffer[usedLen];
    auto *head = reinterpret_cast<PrivateDataHead*>((char*)pBuffer);
    uint32_t bodyLen = infra::ntohl(head->bodyLen);
    int32_t frameLen = bodyLen + sizeof(PrivateDataHead);
    char *body = (char*)buffer + sizeof(PrivateDataHead);

    ///不满一帧数据
    if ((int32_t)len < frameLen) {
        warnf("not enough an frame, frameLen:%d, dalalen:%d\n", frameLen, len);
        return nullptr;
    }

    ///消耗掉一帧数据
    usedLen += frameLen;
    MessagePtr msg(new Message());
    msg->isResponse = head->flag & 0x40 ? true : false;
    msg->sequence  = infra::ntohl(head->sequence);
    msg->sessionId = infra::ntohl(head->sessionId);

    ///媒体数据
    if (head->type == MESSAGE_TYPE_MEDIA) {
        msg->isMediaData = true;
        msg->mediaData = head->buf;
        msg->mediaDataLen = bodyLen;
        return msg;
    } else if (head->type == MESSAGE_TYPE_RPC) {
        infra::Buffer buffer(bodyLen);
        buffer.putData(body, bodyLen);
        rpc_server_->route(buffer);
        return nullptr;
    } else {
        ///信令数据
        Json::Value root = Json::nullValue;
        Json::String err;
        Json::CharReaderBuilder readbuilder;
        std::unique_ptr<Json::CharReader> jsonreader(readbuilder.newCharReader());
        jsonreader->parse(head->buf, head->buf + bodyLen, &root, &err);
        if (root.empty()) {
            return nullptr;
        }

        infof("recv data content:\n%s", root.toStyledString().c_str());
        msg->isResponse = root.isMember("code");
        msg->method     = root.isMember("method") ? root["method"].asString() : "";
        msg->data       = root.isMember("data") ? root["data"] : Json::nullValue;
    }

    return msg;
}

void PrivSessionBase::process(MessagePtr &request){
    PrivSessionBase *session = this;
    if (session == nullptr && !request->isResponse) {
        Json::Value data = Json::Value::null;
        request->code = 100;
        request->message = "invalid session id";
        request->data = data;
        response(request);
        return;
    } 

    if (request->isResponse) {
        warnf("priv resopnse not deal\n");  ///应答暂时不处理
        return;
    }

    session->onRequest(request);
}

int32_t PrivSessionBase::sendStream(const char* data, int32_t dataLen, int32_t reserve) {
    std::lock_guard<std::mutex> guard(mSendMutex);
    if (reserve == sizeof(PrivateDataHead)) {  ///填充头
        PrivateDataHead *head = reinterpret_cast<PrivateDataHead*>((char*)data);
        head->tag[0] = '@';
        head->tag[1] = '@';
        head->tag[2] = '@';
        head->tag[3] = '@';
        head->version = 0x10;
        head->flag = 0x80;
        head->type = 0x01;                       ///媒体数据
        head->encrypt  = 0x00;
        head->sequence  = infra::htons(mSequence++);    ///转网络字节序
        head->sessionId = infra::htonl(mSessionId);
        head->bodyLen   = infra::htonl(dataLen - reserve);
    } else {
        //errorf("reserve(%d) != sizeof(PrivateDataHead)(%d)\n", reserve, sizeof(PrivateDataHead));
    }

    if (mProxy){
        return mProxy->sendData(data, dataLen);
    }

    int32_t sendLen = 0;
    int32_t toSendLen = dataLen;

    do {
        int32_t len = send(data, toSendLen);
        if (len < 0){
            errorf("send len %d error\n", toSendLen);
            break;
        } else if (len == 0) {
            warnf("send len = 0\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        toSendLen -= len;
        sendLen += len;
    } while (toSendLen > 0);

    return sendLen;
}

int32_t PrivSessionBase::sendRequest(Json::Value &body) {
    std::lock_guard<std::mutex> guard(mSendMutex);
    std::string data = body.toStyledString();
    uint32_t dataLen = (uint32_t)data.length();
    uint32_t bufferLen = (uint32_t)data.length() + sizeof(PrivateDataHead);
    std::shared_ptr<uint8_t> buffer(new uint8_t[bufferLen]);
    if (buffer.get() == nullptr) {
        return -1;
    }
    PrivateDataHead *head = reinterpret_cast<PrivateDataHead*>((char*)buffer.get());
    head->tag[0] = '@';
    head->tag[1] = '@';
    head->tag[2] = '@';
    head->tag[3] = '@';
    head->version = 0x10;
    head->flag = 0x80;
    head->type = 0x00;                         ///信令数据
    head->encrypt  = 0x00;
    head->sequence  = infra::htons(mSequence++);    ///转网络字节序
    head->sessionId = infra::htonl(mSessionId);
    head->bodyLen   = infra::htonl(dataLen);
    memcpy(head->buf, data.c_str(), dataLen);
    if (mProxy) {
        return mProxy->sendData((const char*)buffer.get(), bufferLen);
    } else {
        return sendCommand((const char*)buffer.get(), bufferLen);
    }
}
