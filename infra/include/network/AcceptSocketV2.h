/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  AcceptSocketV2.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 12:06:59
 * Description :  AcceptSocket的升级版，简化了使用方法
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include <memory>
#include "Socket.h"
#include "TcpSocket.h"
#include "SocketHandler.h"

namespace infra {

using AcceptorCallback = std::function<void(std::shared_ptr<TcpSocket>)>;

class AcceptSocketV2 : public Socket, public SocketHandler {
public:
    AcceptSocketV2() = default;
    virtual ~AcceptSocketV2() = default;

    bool start(const std::string& ip, uint16_t port, AcceptorCallback callback, int32_t backlog = 10);
    bool stop();

private:
    virtual int32_t onRead(int32_t fd) override;
    virtual int32_t onException(int32_t fd) override;

    bool listen(const std::string& ip, uint16_t port, int32_t backlog = 10);
    std::shared_ptr<TcpSocket> accept();

    AcceptorCallback accept_callback_;
};
}