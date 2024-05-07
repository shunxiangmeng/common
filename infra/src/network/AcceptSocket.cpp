/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  AcceptSocket.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 10:50:37
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/network/AcceptSocket.h"
#include "infra/include//Logger.h"
#include "../Errno.h"
#include "infra/include/network/Defines.h"

namespace infra {

AcceptSocket::AcceptSocket() : Socket(SocketType::tcp) {
}

AcceptSocket::~AcceptSocket() {
}

bool AcceptSocket::listen(const std::string& ip, uint16_t port, int32_t backlog) {
    fd_ = (int32_t)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
        errorf("create socket error, errno:%d\n", lastErrno(true));
        return false;
    }

    Socket::setReuseable(true, true);
    Socket::setNoblocked(true);
    Socket::setCloExec(true);

    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = inet_addr(ip.data());
    if (::bind(fd_, (struct sockaddr*)&local_addr, sizeof(struct sockaddr)) < 0) {
        errorf("bind failed! %s:%d\n", ip.data(), port);
        close(fd_);
        return false;
    }

    if (::listen(fd_, backlog) < 0) {
        errorf("Listen %s:%d failed, backlog:%d\n", ip.c_str(), port, backlog);
        close(fd_);
        return false;
    }
    return true;
}

std::shared_ptr<TcpSocket> AcceptSocket::accept() {
    if (fd_ < 0) {
        errorf("acceptsocket not listen\n");
        return nullptr;
    }
    struct sockaddr_in sockaddr;
    socklen_t addr_len = sizeof(sockaddr);
    int32_t client_fd = (int32_t)::accept(fd_, (struct sockaddr*)&sockaddr, &addr_len);
    if (client_fd < 0) {
        errorf("accept error return fd:%d\n", client_fd);
        return nullptr;
    }

    std::shared_ptr<TcpSocket> new_client_tcp = std::make_shared<TcpSocket>(client_fd);
    new_client_tcp->setConnectState(TcpSocket::connected);
    return new_client_tcp;
}

}