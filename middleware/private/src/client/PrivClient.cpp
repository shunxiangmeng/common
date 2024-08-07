/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivateClient.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-27 10:00:59
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivClient.h"
#include "infra/include/Logger.h"
#include "../PrivMessage.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "../ulucuframe/UlucuPack.h"

#define JSON_IS_TYPE(type) is##type()
#define JSON_AS_TYPE(type) as##type()
#define GET_AND_CHECK_PARAMETER(__result, data, x, type) \
    {                                                    \
        if (!data.isMember(x)) {                         \
            errorf("missing parameter %s\n", x);         \
            return false;                                \
        }                                                \
        if (!data[x].JSON_IS_TYPE(type)) {               \
            errorf("%s wrong parameter type, need %s\n", x, #type); \
            return false;                                \
        }                                                \
        __result = data[x].JSON_AS_TYPE(type);           \
    }

std::shared_ptr<IPrivClient> IPrivClient::create() {
    auto ptr = std::make_shared<PrivClient>();
    return ptr;
}

PrivClient::PrivClient() : buffer_(32 * 1024), rpc_client_(this) {
    initMethodList();
}

PrivClient::~PrivClient() {
}

#define REGISTER_METHOND_FUNC(x) registerMethodFunc(#x, &PrivClient::x, this)
void PrivClient::initMethodList() {
    REGISTER_METHOND_FUNC(event);
}

bool PrivClient::call(std::string key, std::shared_ptr<Message>& message) {
    auto it = map_invokers_.find(key);
    if (it == map_invokers_.end()) {
        message->code = 1;
        message->message = "not support this method";
        errorf("not support method:%s\n", message->method.c_str());
    }
    else {
        return it->second(message);
    }
    return false;
}

bool PrivClient::connect(const char* server_ip, uint16_t server_port) {
    if (!sock_) {
        sock_ = std::make_shared<infra::TcpSocket>();
    }
    if (sock_->isConnected()) {
        return true;
    }
    if (!sock_->connect(std::string(server_ip), server_port, false)) {
        errorf("priv_client connect to %s:%d failed\n", server_ip, server_port);
        sock_.reset();
        return false;
    }
    sock_->setNoblocked(true);
    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(sock_->getHandle(), event_type, shared_from_this())) {
        errorf("initial error\n");
        sock_.reset();
        return false;
    }

    login();
    return true;
}

std::shared_ptr<Message> PrivClient::parse() {
    int32_t frame_len = 0;
    auto message = parseBuffer(buffer_.data(), buffer_.size(), frame_len);
    if (!message) {
        return message;
    }

    int32_t left_len = buffer_.size() - frame_len;
    if (left_len > 0) {
        memcpy(buffer_.data(), buffer_.data() + frame_len, left_len);
        buffer_.setSize(left_len);
    } else {
        buffer_.setSize(0);
    }
    tracef("after parse mBufferDataLen:%d\n", left_len);
    return message;
}


void PrivClient::process(std::shared_ptr<Message> &message) {
    if (message->isMediaData) {
        onMediaFrame(message);
        return;
    }
    if (message->isResponse) {
        onResponse(message);
    } else {
        onRequest(message);
    }
}

void PrivClient::process(const char* buffer, int32_t size) {
    int32_t used_len = 0;
    std::shared_ptr<Message> message = parseBuffer(buffer, size, used_len);
    if (message) {
        process(message);
    }
}

void PrivClient::onResponse(std::shared_ptr<Message> &message) {
    uint32_t sequence = message->sequence;
    std::lock_guard<std::recursive_mutex> lock(future_map_mutex_);
    auto it = future_map_.find(sequence);
    if (it != future_map_.end()) {
        CallResult result;
        result.error_code = PrivErrorCode::SUCCESS;
        result.error_message = message->message;
        result.data = message->data;

        auto& f = future_map_[sequence];
        f->set_value(std::move(result));
        future_map_.erase(it);
    }
}

void PrivClient::onRequest(std::shared_ptr<Message>& message) {
    //infof("request method %s\n", message->method.c_str());
    call(message->method, message);
}

void PrivClient::onMediaFrame(std::shared_ptr<Message> &message) {
    MediaFrame frame = UlucuPack::getMediaFrameFromBuffer(message->mediaData, message->mediaDataLen);
    if (frame.empty()) {
        return;
    }
    media_signal_(frame.getMediaFrameType(), frame);
}

#if 0
int32_t PrivClient::onRead(int32_t fd) {
    do {
        CommonHeader common_header;
        int32_t common_header_len = sizeof(common_header);
        int32_t ret = sock_->recv((char*)&common_header, common_header_len);
        if (ret < 0) {
            warnf("socket disconnect\n");
            onDisconnect();
            return -1;
        } else if (ret == 0) {
            return 0;
        }
        if (ret != common_header_len) {
            errorf("read common_header failed, ret:%d\n", ret);
            return 0;
        }

        int32_t frame_len = common_header.body_len;
        if (common_header.magic == MAGIC_RPC) {
            frame_len += sizeof(rpc_header);
        } else if (common_header.magic == MAGIC_PRIV) {
            frame_len = infra::ntohl(common_header.body_len);
            frame_len += sizeof(PrivateDataHead);
        } else {
            warnf("unknown magic 0x%08x\n", common_header.magic);
        }

        infra::Buffer buffer(frame_len);
        buffer.putData((char*)&common_header, common_header_len);

        char *to_read_buffer = buffer.data() + buffer.size();
        int32_t to_read_length = frame_len - common_header_len;
        ret = sock_->recv(to_read_buffer, to_read_length);
        if (ret < 0) {
            warnf("socket disconnect\n");
            return -1;
        }
        if (ret < to_read_length) {
            warnf("readed len:%d < to_read_length:%d, discard\n", ret, to_read_length);
            return 0;
        }
        buffer.setSize(buffer.size() + ret);

        if (common_header.magic == MAGIC_RPC) {
            processRpc(buffer);
        } else if (common_header.magic == MAGIC_PRIV) {
            process(buffer);
        }
    } while (true);
    return 0;
}
#endif

bool PrivClient::ensureRead(int32_t &readed_size) {
    char *to_read_buffer = buffer_.data() + buffer_.size();
    int32_t to_read_length = buffer_.leftSize();
    int32_t ret = sock_->recv(to_read_buffer, to_read_length);
    if (ret < 0) {
        warnf("socket disconnect\n");
        return false;
    }
    if (ret >= 0 && ret < to_read_length) {
        buffer_.setSize(buffer_.size() + ret);
        readed_size += ret;
        return true;
    }
    buffer_.setSize(buffer_.size() + ret);
    readed_size += ret;

    // 2倍扩容
    int32_t to_expend_size = buffer_.capacity() * 2;
    if (to_expend_size > 2 * 1024 * 1024) {
        warnf("expand buffer size to %d error\n", to_expend_size);
        return false;
    }
    warnf("expand buffer size to %d\n", to_expend_size);
    if (!buffer_.ensureCapacity(to_expend_size)) {
        warnf("expand buffer size to %d error\n", to_expend_size);
        return false;
    }
    return ensureRead(readed_size);
}

int32_t PrivClient::onRead(int32_t fd) {
    if (fd != sock_->getHandle()) {
        errorf("socketFd(%d) != getHandle(%d)\n", fd, sock_->getHandle());
        return -1;
    }
    int32_t readed_size = 0;
    if (!ensureRead(readed_size)) {
        errorf("ensureRead failed, readed_size:%d\n", readed_size);
        onDisconnect();
        return false;
    }
    //infof("readed_size:%d.............\n", readed_size);
    int32_t used_len = 0;
    int32_t left_size = buffer_.size();
    do {
        char* data = buffer_.data();
        data += used_len;

        int32_t frame_len = 0;
        CommonHeader *common_header = (CommonHeader*)data;
        if (common_header->magic == MAGIC_RPC) {
            frame_len = common_header->body_len;
            frame_len += sizeof(rpc_header);
        } else if (common_header->magic == MAGIC_PRIV) {
            frame_len = infra::ntohl(common_header->body_len);
            frame_len += sizeof(PrivateDataHead);
        } else {
            warnf("unknown magic 0x%08x, left_size:%d\n", common_header->magic, left_size);
            frame_len = 1;
        }

        if (frame_len > left_size) {
            memcpy(buffer_.data(), data, left_size);
            //tracef("frame_len %d > left_size %d\n", frame_len, left_size);
            break;
        }

        if (common_header->magic == MAGIC_RPC) {
            processRpc(data, frame_len);
        } else if (common_header->magic == MAGIC_PRIV) {
            process(data, frame_len);
        }
        used_len += frame_len;
        left_size -= frame_len;

        if (left_size > 0 && left_size < sizeof(CommonHeader)) {
            memcpy(buffer_.data(), data, left_size);
            warnf("left_size %d enough head size %d\n", left_size, sizeof(CommonHeader));
            break;
        }
    } while (left_size > 0);
    buffer_.resize(left_size);
    return 0;
}

int32_t PrivClient::onException(int32_t fd) {
    errorf("priv_client socket error\n");
    onDisconnect();
    return 0;
}

void PrivClient::onDisconnect() {
    warnf("to remove fd:%d\n", sock_->getHandle());
    if (!infra::NetworkThreadPool::instance()->delSocketEvent(sock_->getHandle(), shared_from_this())) {
        errorf("delSocketEvent error\n");
    }
    sock_.reset();
}

/*
int32_t PrivClient::sendRequest(Json::Value &body) {
    infra::Buffer buffer = makeRequest(body);
    std::lock_guard<std::mutex> guard(send_mutex_);
    return sock_->send((const char*)buffer.data(), buffer.size());
}
*/

infra::Buffer PrivClient::makeRequest(uint32_t sequence, Json::Value &body) {
    std::string data = body.toStyledString();
    uint32_t data_len = (uint32_t)data.length();
    uint32_t buffer_len = (uint32_t)data.length() + sizeof(PrivateDataHead);
    infra::Buffer buffer(buffer_len);
    PrivateDataHead *head = reinterpret_cast<PrivateDataHead*>((char*)buffer.data());
    head->magic = MAGIC_PRIV;
    head->version = 0x10;
    head->flag = 0x80;
    head->type = 0x00;                         ///信令数据
    head->encrypt  = 0x00;
    head->sequence   = infra::htonl(sequence);    ///转网络字节序
    head->session_id = infra::htonl(session_id_);
    head->body_len   = infra::htonl(data_len);
    memcpy((char*)buffer.data() + sizeof(PrivateDataHead), data.c_str(), data_len);
    buffer.setSize(buffer_len);
    return buffer;
}

CallResult PrivClient::syncRequest(Json::Value &body) {
    auto releaseCall = [this](uint32_t resquence) {
        std::lock_guard<std::recursive_mutex> guard(future_map_mutex_);
        auto it = future_map_.find(resquence);
        if (it != future_map_.end()) {
            future_map_.erase(it);
        }
    };

    auto async_call_result = asyncRequest(body);
    auto wait_result = async_call_result->second.wait_for(std::chrono::milliseconds(REQUEST_TIMEOUT));
    if (wait_result == std::future_status::timeout) {
        releaseCall(async_call_result->first);
        CallResult result;
        result.error_code = PrivErrorCode::TIMEOUT;
        result.error_message = "timeout";
        return result;
    }
    CallResult result = async_call_result->second.get();
    releaseCall(async_call_result->first);
    return result;
}

std::shared_ptr<AsyncCallResult> PrivClient::asyncRequest(Json::Value &body) {
    auto promise = std::make_shared<std::promise<CallResult>>();
    std::future<CallResult> future = promise->get_future();

    uint32_t sequence = 0;
    {
        std::lock_guard<std::recursive_mutex> guard(future_map_mutex_);
        sequence = sequence_++;
        future_map_.emplace(sequence, std::move(promise));
    }

    infra::Buffer buffer = makeRequest(sequence, body);
    int32_t send_len = sock_->send((const char*)buffer.data(), buffer.size());
    if (send_len < 0) {
        CallResult result;
        result.error_message = "send failed";
        promise->set_value(std::move(result));
    }
    std::shared_ptr<AsyncCallResult> async_call_result = std::make_shared<AsyncCallResult>();
    async_call_result->first = sequence;
    async_call_result->second = std::move(future);
    return async_call_result;
}

int32_t PrivClient::sendRpcData(infra::Buffer& buffer) {
    if (!sock_ || !sock_->isConnected()) {
        return -1;
    }
    return sock_->send((const char*)buffer.data(), buffer.size());
}

void PrivClient::sendKeepAlive() {
    Json::Value root;
    root["method"] = "keepAlive";
    //sendRequest(root);
}

bool PrivClient::login() {
    Json::Value root;
    root["method"] = "login";
    return syncRequest(root).success();
}

bool PrivClient::testSyncCall() {
    for (int i = 0; i < 2; i++) {
        auto result = rpc_client_.call<std::vector<int32_t>>("echo", 1 + i, 2);
        printf("result.size():%ld\n", (*result).size());
        for (auto& it : *result) {
            printf("%ld ", it);
        }
        printf("\n");
    }
    return true;
}

void PrivClient::processRpc(const char* buffer, int32_t size) {
    rpc_client_.processResponse(buffer, size);
}

bool PrivClient::startPreview(int32_t channel, int32_t sub_channel, OnFrameProc onframe) {
    Json::Value root;
    root["method"] = "start_preview";
    root["data"]["channel"] = channel;
    root["data"]["sub_channel"] = sub_channel;
    if (!syncRequest(root).success()) {
        errorf("start preview failed\n");
        return false;
    }
    preview_channel_ = channel;
    preview_sub_channel_ = sub_channel;
    int32_t ret = media_signal_.attach(onframe);
    if (ret < 0) {
        errorf("media_signal attach failed ret:%d\n", ret);
        return false;
    }
    return true;
}

bool PrivClient::stopPreview(OnFrameProc onframe) {
    Json::Value root;
    root["method"] = "stop_preview";
    root["data"]["channel"] = preview_channel_;
    root["data"]["sub_channel"] = preview_sub_channel_;
    if (!syncRequest(root).success()) {
        errorf("stop preview failed\n");
        return false;
    }
    int32_t ret = media_signal_.detach(onframe);
    if (ret < 0) {
        errorf("media_signal detach failed ret:%d\n", ret);
        return false;
    }
    return true;
}

bool PrivClient::subscribeEvent(const char* event, EventFunction event_callback) {
    Json::Value root;
    root["method"] = "subscribe_event";
    root["data"]["event"].append(event);
    if (!syncRequest(root).success()) {
        errorf("stop preview failed\n");
        return false;
    }
    std::lock_guard<std::mutex> lock(event_callbacks_map_mutex_);
    auto it = event_callbacks_.find(event);
    if (it == event_callbacks_.end()) {
        event_callbacks_[event] = event_callback;
    }
    return true;
}

bool PrivClient::event(std::shared_ptr<Message>& message) {
    std::lock_guard<std::mutex> lock(event_callbacks_map_mutex_);
    auto it = event_callbacks_.find(message->event_name);
    if (it != event_callbacks_.end()) {
        it->second(message->data);
    
    }
    return true;
}

bool PrivClient::getVideoFormat(std::string &format) {
    Json::Value root;
    root["method"] = "get_video_format";
    CallResult result = syncRequest(root);
    if (!result.success()) {
        errorf("get_video_format failed\n");
        return false;
    }
    GET_AND_CHECK_PARAMETER(format, result.data, "video_format", String);
    return true;
}

bool PrivClient::setVideoFormat(std::string &format) {
    Json::Value root;
    root["method"] = "set_video_format";
    root["data"]["video_format"] = format;
    CallResult result = syncRequest(root);
    if (!result.success()) {
        errorf("set_video_format failed\n");
        return false;
    }
    return true;
}

bool PrivClient::getVideoConfig(Json::Value &video_config) {
    Json::Value root;
    root["method"] = "get_video_config";
    CallResult result = syncRequest(root);
    if (!result.success()) {
        errorf("get_video_config failed\n");
        return false;
    }
    video_config = result.data;
    tracef("%s\n", video_config.toStyledString().data());
    return true;
}

bool PrivClient::setVideoConfig(Json::Value &video_config) {
    Json::Value root;
    root["method"] = "set_video_config";
    root["data"] = video_config;
    CallResult result = syncRequest(root);
    if (!result.success()) {
        errorf("set_video_config failed\n");
        return false;
    }
    return true;
}
