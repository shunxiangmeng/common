/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UdpSocket.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 10:51:29
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "UdpSocket.h"
#include "../Errno.h"
#include "../Logger.h"

namespace infra {
    
UdpSocket::UdpSocket() : Socket(SocketType::udp, -1) {
}

UdpSocket::~UdpSocket() {
}

int32_t UdpSocket::open(const std::string& ip, uint16_t port, bool reuseaddr) {
    fd_ = (int32_t)::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd_ < 0) {
        errorf("create socket failed! errno:%d\n", lastErrno(true));
        return -1;
    }

    if (reuseaddr) {
        Socket::setReuseable(true, true);
    }
    Socket::setNoblocked(true);
    Socket::setCloExec(true);

    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = inet_addr(ip.data());
    if (::bind(fd_, (struct sockaddr*)&local_addr, sizeof(struct sockaddr)) < 0) {
        errorf("bind failed! %s:%d, errno:%d\n", ip.data(), port, lastErrno(true));
        close(fd_);
        fd_ = -1;
        return -1;
    }
    local_port_ = port;
    return 0;
}

int32_t UdpSocket::send(const char *buffer, int32_t len, struct sockaddr_in &remote) {
    if (buffer == nullptr || len <= 0) {
        return -1;
    }

    int32_t ret = ::sendto(fd_, buffer, len, 0, (struct sockaddr*)&remote, sizeof(sockaddr_in));
    if (ret < 0) {
        if (lastErrno(true) == EWOULDBLOCK || lastErrno(true) == EINTR) {
            return 0;
        }
        errorf("sendto failed! errno:%d\n", lastErrno(true));
        return ret;
    }
    return ret;
}

int32_t UdpSocket::recv(char *buffer, int32_t len, struct sockaddr_in *remote) {
    if (buffer == nullptr || len <= 0) {
        return -1;
    }

    int32_t ret = -1;
    if (remote) {
        socklen_t addrLen = sizeof(sockaddr_in);
        ret = ::recvfrom(fd_, buffer, len, 0, (struct sockaddr*)&remote, &addrLen);
    } else {
        ret = ::recvfrom(fd_, buffer, len, 0, nullptr, nullptr);
    }

    if (ret < 0) {
        if (lastErrno(true) == EWOULDBLOCK || lastErrno(true) == EINTR) {
            return 0;
        }
        errorf("recv from failed! errno:%d\n", lastErrno(true));
        return ret;
    }
    return ret;
}

uint16_t UdpSocket::getLocalPort() {
    return local_port_;
}

int32_t UdpSocket::setRemoteAddr(struct sockaddr_in &remote) {
    int32_t ret = ::connect(fd_, (struct sockaddr*)&remote, sizeof(struct sockaddr));
    if (ret < 0) {
        #ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            errorf("connect failed! errno: %d\n", WSAGetLastError());
            return -1;
        }
        #else
        if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
            errorf("connect failed! %s\n", STRERROR);
            return -1;
        }
        #endif
    }
    return 0;
}

int32_t UdpSocket::send(const char *buffer, int32_t length) {
    if (buffer == nullptr) {
        return -1;
    }
    int32_t ret = ::send(fd_, buffer, length, 0);
    if (ret < 0) {
        if (lastErrno(true) == EWOULDBLOCK || lastErrno(true) == EINTR) {
            return 0;
        }
        errorf("send %d failed! errno:%d\n", length, lastErrno(true));
        return ret;
    }
    return ret;
}

} // namespace infra
