/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 11:39:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "HttpServer.h"
#include "infra/include/Logger.h"
#include "Connection.h"

IHttpServer* IHttpServer::instance() {
    static HttpServer s_http_server;
    return &s_http_server;
}

HttpServer::HttpServer() {
    acceptor_ = std::make_shared<infra::AcceptSocketV2>();
}

HttpServer::~HttpServer() {
}

void HttpServer::config(Config& config) {
    handler_.config(config);
}

bool HttpServer::start(uint16_t port) {
    bool opened = acceptor_->start("0.0.0.0", port, [this] (std::shared_ptr<infra::TcpSocket> new_tcp_socket) {
        std::string ip;
        uint16_t port;
        new_tcp_socket->getRemoteAddress(ip, port);
        infof("new http connection fd:%d from %s:%d\n", new_tcp_socket->getHandle(), ip.data(), port);
        auto new_connection = std::make_shared<Connection>(new_tcp_socket, connection_manager_, handler_);
        connection_manager_.addNewConnection(new_connection);
        new_connection->start();
    });

    if (!opened) {
        errorf("start httpserver on port %d failed\n", port);
    } else {
        infof("start httpserver on port %d\n", port);
    }
    return opened;
}

bool HttpServer::stop() {
    connection_manager_.clear();
    acceptor_->stop();
    return true;
}

bool HttpServer::restart(uint16_t port) {
    return stop() && start(port);
}

bool HttpServer::registerHandler(HttpMethod method, const char* url, httpHandler&& handler) {
    return handler_.registerHandler(method, url, std::move(handler));
}