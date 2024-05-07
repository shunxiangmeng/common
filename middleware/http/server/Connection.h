/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Connection.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 14:12:18
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "infra/include/network/TcpSocket.h"
#include "infra/include/network/SocketHandler.h"
#include "ConnectionManager.h"
#include "Request.h"
#include "Reply.h"
#include "HttpHandler.h"

class Connection : public infra::SocketHandler {
public:
    Connection() = delete;
    Connection(std::shared_ptr<infra::TcpSocket> sock, ConnectionManager& connection_manager, HttpHandler& handler);
    ~Connection();

    int32_t handle() const;
    bool start();
    bool stop();

private:
    virtual int32_t onRead(int32_t fd) override;
    virtual int32_t onException(int32_t fd) override;

    void checkKeepAlive();
    void onDisconnect();
    void response();

private:
    std::shared_ptr<infra::TcpSocket> sock_;
    ConnectionManager& connection_manager_;
    Request request_;
    Reply reply_;
    HttpHandler& handler_;
    bool keep_alive_ = false;
};