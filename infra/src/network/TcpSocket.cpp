/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TcpSocket.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-17 23:16:20
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/network/TcpSocket.h"
#include "infra/include/Logger.h"
#include "../Errno.h"
#include "infra/include/network/Defines.h"

namespace infra {

TcpSocket::TcpSocket() : Socket(SocketType::tcp), conncect_state_(unconnected) {
}

TcpSocket::TcpSocket(int fd) : Socket(SocketType::tcp, fd), conncect_state_(unconnected) {
}

TcpSocket::~TcpSocket() {
}

bool TcpSocket::connect(const std::string& remote_ip, uint16_t remote_port, bool noblock, const std::string& local_ip, uint16_t local_port) {
    if (conncect_state_ == connected) {
        errorf("socket fd %d has already connected\n", fd_);
        return false;
    }

    bool is_ip = isIPAddress(remote_ip);
    bool is_domain = isDomainName(remote_ip);

    if ((is_ip && is_domain) || (!is_ip && !is_domain)) {
        errorf("ip error: %s\n", remote_ip.data());
        return false;
    }

    std::string real_remote_ip = remote_ip;
    if (is_domain) {
        tracef("query domain: %s\n", remote_ip.data());
        struct addrinfo hints = {0}, * result;
        int ret = getaddrinfo(remote_ip.data(), nullptr, &hints, &result);
        if (ret != 0) {
            errorf("getaddrinfo error:%d\n", ret);
            return false;
        }
        struct sockaddr_in *sockaddr_ipv4;
        int count = 0;
        int ipbufferlength = 0;
        for (struct addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
            tracef("getaddrinfo response %d\n", count++);
            tracef("Flags: 0x%x\n", ptr->ai_flags);
            switch (ptr->ai_family) {
                case AF_UNSPEC:
                    warnf("Unspecified\n");
                    break;
                case AF_INET:
                    tracef("AF_INET (IPv4)\n");
                    sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
                    real_remote_ip = inet_ntoa(sockaddr_ipv4->sin_addr);
                    tracef("IPv4 address: %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));
                    break;
                case AF_INET6:
                    tracef("AF_INET6 (IPv6)\n");
                    #if 0
                    // the InetNtop function is available on Windows Vista and later
                    // sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
                    // printf("\tIPv6 address %s\n",
                    //    InetNtop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipstringbuffer, 46) );
                    
                    // We use WSAAddressToString since it is supported on Windows XP and later
                    sockaddr_ip = (LPSOCKADDR) ptr->ai_addr;
                    // The buffer length is changed by each call to WSAAddresstoString
                    // So we need to set it for each iteration through the loop for safety
                    ipbufferlength = 46;
                    ret = WSAAddressToString(sockaddr_ip, (DWORD) ptr->ai_addrlen, NULL, ipstringbuffer, &ipbufferlength );
                    if (ret) {
                        errorf("WSAAddressToString failed with %u\n", WSAGetLastError());
                    } else {    
                        tracef("IPv6 address %s\n", ipstringbuffer);
                    }
                    #endif
                    break;
                default:
                    warnf("Other %ld\n", ptr->ai_family);
                    break;
            }
            freeaddrinfo(result);
            break;
        }
    }

    fd_ = (int32_t)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_ < 0) {
        errorf("create socket failed! fd:%d\n", fd_);
        return false;
    }
    infof("create socket fd:%d\n", fd_);

    if (local_ip != "") {
        struct sockaddr_in local_addr = {0};
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(local_port);
        local_addr.sin_addr.s_addr = inet_addr(local_ip.data());
        if (::bind(fd_, (struct sockaddr*)&local_addr, sizeof(struct sockaddr)) < 0) {
            errorf("bind failed! %s:%d\n", local_ip.data(), local_port);
            close(fd_);
            fd_ = -1;
            return false;
        }
    }

    Socket::setReuseable(true);
    Socket::setNoblocked(noblock);
    Socket::setCloExec(true);

    struct sockaddr_in remote_addr = {0};
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(real_remote_ip.data());
    if (::connect(fd_, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == 0) {
        //同步连接成功
        setConnectState(ConnectState::connected);
        return true;
    }

    #ifdef _WIN32
    if (WSAGetLastError() != WSAEWOULDBLOCK) {
        errorf("connect %s:%d failed! errno: %d\n", real_remote_ip.data(), remote_port, WSAGetLastError());
        close(fd_);
        fd_ = -1;
        return false;
    }
    #else
    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
        errorf("connect failed! %s\n", STRERROR);
        close(fd_);
        fd_ = -1;
        return false;
    }
    #endif
    remote_ip_ = real_remote_ip;
    remote_port_ = remote_port;
    return true;
}

bool TcpSocket::isConnected() {
    return conncect_state_ == connected;
}

TcpSocket::ConnectState TcpSocket::getConnectState() const {
    return conncect_state_;
}

void TcpSocket::setConnectState(TcpSocket::ConnectState state) {
    conncect_state_ = state;
}

bool TcpSocket::setKeepalive(bool keepalive, int interval, int idle, int times) {
    int opt = keepalive ? 1 : 0;
    int ret = setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        errorf("setsockopt SO_KEEPALIVE failed\n");
        return false;
    }

#ifndef _WIN32
#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
  #define SOL_TCP IPPROTO_TCP
#endif
#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
  #define TCP_KEEPIDLE TCP_KEEPALIVE
#endif
    // Set the keep-alive parameters
    if (keepalive && interval > 0) {
        ret = setsockopt(fd_, SOL_TCP, TCP_KEEPIDLE, (char *)&idle, static_cast<socklen_t>(sizeof(idle)));
        if (ret == -1) {
            errorf("setsockopt TCP_KEEPIDLE failed\n");
            return false;
        }
        ret = setsockopt(fd_, SOL_TCP, TCP_KEEPINTVL, (char *)&interval, static_cast<socklen_t>(sizeof(interval)));
        if (ret == -1) {
            errorf("setsockopt TCP_KEEPINTVL failed\n");
            return false;
        }
        ret = setsockopt(fd_, SOL_TCP, TCP_KEEPCNT, (char *)&times, static_cast<socklen_t>(sizeof(times)));
        if (ret == -1) {
            errorf("setsockopt TCP_KEEPCNT failed\n");
            return false;
        }
    }
#endif
    return true;
}

bool TcpSocket::setNoDelay(bool nodelay) {
    int opt = nodelay ? 1 : 0;
    int ret = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret < 0) {
        errorf("setsockopt TCP_NODELAY failed\n");
        return false;
    }
    return true;
}

int32_t TcpSocket::send(const char *data, int32_t length) {
    if (data == nullptr) {
        return -1;
    }
    int32_t ret = ::send(fd_, data, length, 0);
    if (ret < 0 && (lastErrno(true) == EWOULDBLOCK || lastErrno(true) == EINTR)) {
        return 0;
    } else if (ret < 0) {
        errorf("send size %d failed! errno:%d\n", length, lastErrno(true));
        return -1;
    }
    return ret;
}

int32_t TcpSocket::recv(char* buffer, int32_t length) {
    if (buffer == nullptr) {
        return -1;
    }
    int32_t ret = ::recv(fd_, buffer, length, 0);
    if (ret < 0) {
        int32_t last_errno = lastErrno(true);
        if (last_errno == EWOULDBLOCK || last_errno == EINTR) {
            return 0;
        }
#ifdef _WIN32
        if (last_errno == WSAEWOULDBLOCK) {
            return 0;
        }
#endif
        errorf("recv failed, error:%d\n", lastErrno(true));
        return -1;
    } else if (ret == 0) {
        warnf("socket %d disconnected\n", fd_);
        return -1;
    }
    return ret;
}

int32_t TcpSocket::writeV(const struct iovec *vector, int32_t count) {
    if (vector == nullptr) {
        return -1;
    }
#ifndef _WIN32
    int32_t ret = ::writev(fd_, vector, count);
    if (ret < 0 && (lastErrno(true) == EAGAIN || lastErrno(true) == EWOULDBLOCK || lastErrno(true) == EINTR)) {
        return 0;
    } else if (ret < 0) {
        errorf("send failed! %d\n", lastErrno(true));
        return -1;
    }
    return ret;
#else
    return 0;
#endif
}

bool TcpSocket::getRemoteAddress(std::string &ip, uint16_t &port) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(fd_, (struct sockaddr *)&addr, &addr_len) == 0) {
        ip = inet_ntoa(addr.sin_addr);
        port = ntohs(addr.sin_port);
        return true;
    }
    return false;
}

}