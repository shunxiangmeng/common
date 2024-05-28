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

PrivClient::PrivClient() : buffer_(8 * 1024) {
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

}

int32_t PrivClient::onRead(int32_t fd) {
    char *recv_buffer = buffer_.data() + buffer_.size();
    int32_t to_recv_length = buffer_.leftSize();
    int32_t ret = sock_->recv(recv_buffer, to_recv_length);
    if (ret > 0) {
        std::shared_ptr<Message> message;
        do {
            message = parse();
            if (message) {
                process(message);
            }
        } while(message && buffer_.size() > 0);
    } else if (ret < 0) {
        warnf("socket disconnect\n");
        return -1;
    }
    return 0;
}

int32_t PrivClient::onException(int32_t fd) {
    errorf("priv_client socket error\n");
    return 0;
}


