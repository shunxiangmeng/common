/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  AcceptSocket.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 10:50:32
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "Socket.h"
#include "TcpSocket.h"
#include <memory>

namespace infra {
class AcceptSocket : public Socket {
public:
    AcceptSocket();
    virtual ~AcceptSocket();

    bool listen(const std::string& ip, uint16_t port, int32_t backlog = 10);

    std::shared_ptr<TcpSocket> accept();
};
}
