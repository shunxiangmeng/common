/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Socket.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-16 22:55:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <regex>
#include "infra/include/network/Socket.h"
#include "infra/include/Logger.h"
#include "infra/include/network/Defines.h"
#include "../Errno.h"
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h> 
#endif

namespace infra {
Socket::Socket() : fd_(-1), type_(unknown), blocked_(false) {
}

Socket::Socket(SocketType type, int32_t fd) : fd_(fd), type_(type), blocked_(false) {
}

Socket::~Socket() {
    if (fd_ > 0) {
        //infof("close socket fd %d\n", fd_);
        close(fd_);
        fd_ = -1;
    }
}

void Socket::close(bool notused) {
/*    if (fd_ > 0) {
        warnf("close socket fd %d\n", fd_);
        close(fd_);
        fd_ = -1;
    }
*/
}

int32_t Socket::getHandle() const {
    return fd_;
}

Socket::SocketType Socket::getType() const {
    return type_;
}

bool Socket::isBlock() const {
    return blocked_;
}

bool Socket::valid() const {
    return fd_ > 0;
}

bool Socket::setSendBuffer(int32_t length) {
    int32_t ret = ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, (char *)&length, sizeof(length));
    if (ret < 0) {
        errorf("setsockopt failed! ret:%d\n", ret);
        return false;
    }
    return true;
}

int32_t Socket::getSendBuffer() {
    int32_t length = 0;
    socklen_t size = sizeof(length);
    int ret = ::getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, (char *)&length, &size);
    if (ret < 0){
        errorf("getsockopt failed! ret:%d\n", ret);
        length = -1;
    }
    return length;
}

bool Socket::setReceiveBuffer(int32_t length) {
    int32_t ret = ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, (char*)&length, sizeof(length));
    if (ret < 0) {
        errorf("setsockopt failed! ret:%d\n", errno);
        return false;
    }
    return true;
}

int32_t Socket::getReceiveBuffer() {
    int32_t length = 0;
    socklen_t size = sizeof(length);
    int ret = ::getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, (char *)&length, &size);
    if (ret < 0) {
        errorf("getsockopt failed! ret:%d\n", ret);
        length = -1;
    }
    return length;
}

bool Socket::setReuseable(bool reuse_addr, bool reuse_port) {
    int opt = reuse_addr ? 1 : 0;
    int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret < 0) {
        errorf("setsockopt SO_REUSEADDR failed\n");
        return false;
    }
#if defined(SO_REUSEPORT)
    if (reuse_port) {
        ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));
        if (ret == -1) {
            errorf("setsockopt SO_REUSEPORT failed\n");
            return false;
        }
    }
#endif
    return true;
}

bool Socket::setNoblocked(bool noblock) {
#if defined(_WIN32)
    unsigned long ul = noblock;
#else
    int ul = noblock;
#endif //defined(_WIN32)
    int ret = ioctl(fd_, FIONBIO, &ul); //设置为非阻塞模式
    if (ret == -1) {
        errorf("ioctl FIONBIO failed\n");
        return false;
    }
    blocked_ = !noblock;
    return true;
}

bool Socket::setCloExec(bool on) {
#if !defined(_WIN32)
    int flags = fcntl(fd_, F_GETFD);
    if (flags == -1) {
        errorf("fcntl F_GETFD failed\n");
        return false;
    }
    if (on) {
        flags |= FD_CLOEXEC;
    } else {
        int cloexec = FD_CLOEXEC;
        flags &= ~cloexec;
    }
    int ret = fcntl(fd_, F_SETFD, flags);
    if (ret == -1) {
        errorf("fcntl F_SETFD failed\n");
        return false;
    }
    return true;
#else
    return true;
#endif
}

uint16_t Socket::getLocalPort() {
    struct sockaddr_storage addr_store;
    socklen_t addr_store_len = sizeof(addr_store);
    if (::getsockname(fd_, (struct sockaddr*)&addr_store, &addr_store_len) != 0) {
        errorf("getsockname failed errno:%d\n", lastErrno(true));
        return -1;
    }
    struct sockaddr_in* addr = (struct sockaddr_in*)&addr_store;
    return ntohs(addr->sin_port);
}

bool Socket::isIPAddress(const std::string& str) {
    std::regex ipPattern(R"((\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}))");
    std::smatch match;
    if (std::regex_match(str, match, ipPattern)) {
        for (int i = 1; i <= 4; ++i) {
            int num = std::stoi(match[i].str());
            if (num < 0 || num > 255) {
                return false;
            }
        }
        return true;
    }
    return false;
}
 
bool Socket::isDomainName(const std::string& str) {
    std::regex domainPattern(R"(^([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,}$)");
    return std::regex_match(str, domainPattern);
}

}