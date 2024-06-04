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

std::shared_ptr<IPrivClient> IPrivClient::create() {
    auto ptr = std::make_shared<PrivClient>();
    return ptr;
}

PrivClient::PrivClient() : buffer_(8 * 1024), rpc_client_(this) {
}

PrivClient::~PrivClient() {
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
    if (message->isResponse) {
        uint32_t sequence = message->sequence;
        warnf("deal response, sequence:%d\n", sequence);  ///应答暂时不处理
        return;
    }
    if (message->isMediaData) {
        onMediaFrame(message);
    }
}

void PrivClient::process(infra::Buffer &buffer) {
    int32_t used_len = 0;
    std::shared_ptr<Message> message = parseBuffer(buffer.data(), buffer.size(), used_len);
    if (message) {
        process(message);
    }
}


void PrivClient::onMediaFrame(std::shared_ptr<Message> &message) {
    MediaFrame frame = UlucuPack::getMediaFrameFromBuffer(message->mediaData, message->mediaDataLen);
    if (frame.empty()) {
        return;
    }
    media_signal_(frame.getMediaFrameType(), frame);
}

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
        printf("result.size():%ld\n", result.size());
        for (auto& it : result) {
            printf("%ld ", it);
        }
        printf("\n");
    }
    return true;
}

void PrivClient::processRpc(infra::Buffer &buffer) {
    rpc_client_.processResponse(buffer);
}

bool PrivClient::startPreview(int32_t channel, int32_t sub_channel, OnFrameProc onframe) {
    Json::Value root;
    root["method"] = "start_preview";
    root["channel"] = channel;
    root["sub_channel"] = sub_channel;
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
    //root["channel"] = channel;
    //root["sub_channel"] = sub_channel;
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
