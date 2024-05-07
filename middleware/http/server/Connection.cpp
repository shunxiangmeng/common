/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Connection.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 14:12:56
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Connection.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/thread/WorkThreadPool.h"
#include "infra/include/Logger.h"

Connection::Connection(std::shared_ptr<infra::TcpSocket> sock, ConnectionManager& connection_manager, HttpHandler& handler)
    : sock_(sock), connection_manager_(connection_manager), handler_(handler) {
    tracef("Connection()\n");
}

Connection::~Connection() {
    tracef("~Connection\n");
}

int32_t Connection::handle() const {
    return sock_->getHandle();
}

bool Connection::start() {
    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(sock_->getHandle(), event_type, shared_from_this())) {
        errorf("http connection fd %d error\n", sock_->getHandle());
        return false;
    }
    return true;
}

bool Connection::stop() {
    return infra::NetworkThreadPool::instance()->delSocketEvent(sock_->getHandle(), shared_from_this());
}

int32_t Connection::onRead(int32_t fd) {
    debugf("new request fd:%d\n", fd);
    int32_t ret = request_.readRequest(sock_);
    if (ret > 0) {
        if (!request_.parse()) {
            reply_.setStatus(statusType::bad_request);
            this->response();
            return 0;
        }

        checkKeepAlive();

        // 投递到工作线程，处理和应答
        infra::WorkThreadPool::instance()->async([this]() {
            handler_.handleRequest(request_, reply_);
            this->response();
            request_.reset();
	        reply_.reset();
        });
        
    } else {
        warnf("http client disconnected fd:%d\n", fd);
        onDisconnect();
    }
    return 0;
}

int32_t Connection::onException(int32_t fd) {
    return 0;
}

void Connection::onDisconnect() {
    stop();
    connection_manager_.delConnection(sock_->getHandle());
}

void Connection::response() {
    std::string content = reply_.toBuffer();
    sock_->send(content.data(), (int32_t)content.length());
}

void Connection::checkKeepAlive() {
    auto req_conn_hdr = request_.getHeaderValue("connection");
    if (request_.isHttp11()) {
        // HTTP1.1
        keep_alive_ = req_conn_hdr.empty() || (req_conn_hdr != "close");
    } else {
        keep_alive_ = !req_conn_hdr.empty() && (req_conn_hdr == "keep-alive");
    }

    if (keep_alive_) {
        reply_.addHeader("Connection", "keep-alive");
    } else {
        reply_.addHeader("Connection", "close");
    }
}