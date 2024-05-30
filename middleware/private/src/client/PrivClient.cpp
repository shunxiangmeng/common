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

std::shared_ptr<IPrivClient> IPrivClient::create() {
    auto ptr = std::make_shared<PrivClient>();
    return ptr;
}

PrivClient::PrivClient() : buffer_(8 * 1024), rpc_client_(this) {
    sock_ = std::make_shared<infra::TcpSocket>();
}

PrivClient::~PrivClient() {
}

bool PrivClient::connect(const char* server_ip, uint16_t server_port) {
    if (sock_->isConnected()) {
        return true;
    }
    if (!sock_->connect(std::string(server_ip), server_port, false)) {
        errorf("priv_client connect to %s:%d failed\n", server_ip, server_port);
        return false;
    }
    sock_->setNoblocked(true);
    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(sock_->getHandle(), event_type, shared_from_this())) {
        errorf("initial error\n");
        return false;
    }
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
}

void PrivClient::process(infra::Buffer &buffer) {
    int32_t used_len = 0;
    std::shared_ptr<Message> message = parseBuffer(buffer.data(), buffer.size(), used_len);
    if (message) {
        process(message);
    }
}


int32_t PrivClient::onRead(int32_t fd) {
    do {
        CommonHeader common_header;
        int32_t common_header_len = sizeof(common_header);
        int32_t ret = sock_->recv((char*)&common_header, common_header_len);
        if (ret < 0) {
            warnf("socket disconnect\n");
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
    return 0;
}


int32_t PrivClient::sendRequest(Json::Value &body) {
    infra::Buffer buffer = makeRequest(body);
    std::lock_guard<std::mutex> guard(send_mutex_);
    return sock_->send((const char*)buffer.data(), buffer.size());
}

infra::Buffer PrivClient::makeRequest(Json::Value &body) {
    uint32_t sequence = 0;
    {
        std::lock_guard<std::mutex> guard(sequence_mutex_);
        sequence = sequence_++;
    }

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

int32_t PrivClient::sendRpcData(infra::Buffer& buffer) {
    return sock_->send((const char*)buffer.data(), buffer.size());
}

void PrivClient::sendKeepAlive() {
    Json::Value root;
    root["method"] = "keepAlive";
    sendRequest(root);
}


bool PrivClient::testSyncCall() {
    for (int i = 0; i < 5; i++) {
        auto result = rpc_client_.call<std::vector<int32_t>>("echo", 1 + i, 2);
    }
    return true;
}

void PrivClient::processRpc(infra::Buffer &buffer) {
    rpc_client_.processResponse(buffer);
}