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
    if (message->isResponse) {
        uint32_t sequence = message->sequence;
        warnf("deal response, sequence:%d\n", sequence);  ///应答暂时不处理
        std::unique_lock<std::mutex> lock(cb_mtx_);
        auto& f = future_map_[sequence];
        f->set_value(req_result{ "ok"});
        future_map_.erase(sequence);
        return;
    }
}

int32_t PrivClient::onRead(int32_t fd) {
    char *recv_buffer = buffer_.data() + buffer_.size();
    int32_t to_recv_length = buffer_.leftSize();
    int32_t ret = sock_->recv(recv_buffer, to_recv_length);
    if (ret > 0) {
        buffer_.setSize(buffer_.size() + ret);
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
    head->tag[0] = '@';
    head->tag[1] = '@';
    head->tag[2] = '@';
    head->tag[3] = '@';
    head->version = 0x10;
    head->flag = 0x80;
    head->type = 0x00;                         ///信令数据
    head->encrypt  = 0x00;
    head->sequence  = infra::htonl(sequence);    ///转网络字节序
    head->sessionId = infra::htonl(session_id_);
    head->bodyLen   = infra::htonl(data_len);
    memcpy(head->buf, data.c_str(), data_len);
    buffer.setSize(buffer_len);
    return buffer;
}


void PrivClient::sendKeepAlive() {
    Json::Value root;
    root["method"] = "keepAlive";
    sendRequest(root);
}


bool PrivClient::testSyncCall() {
    call("keep_alive", 1, 2);
    return true;
}