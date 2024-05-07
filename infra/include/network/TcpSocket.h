/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TcpSocket.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 10:51:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "Socket.h"
#ifndef _WIN32
#include <sys/uio.h>
#endif

#define TCP_KEEPALIVE_INTERVAL 30
#define TCP_KEEPALIVE_PROBE_TIMES 9
#define TCP_KEEPALIVE_TIME 120

namespace infra {
class TcpSocket : public Socket {
public:
    TcpSocket();
    TcpSocket(int fd);

    virtual ~TcpSocket();

    /**
     * @brief 连接状态
     */
    typedef enum {
        connected,
        unconnected,
        connectedError,
    } ConnectState;

    /**
     * @brief 连接对端
     * @param remote_ip
     * @param renote_port
     * @param local_ip
     * @param local_port
     * @return
     */
    bool connect(const std::string& remote_ip, uint16_t remote_port, bool noblock = true, const std::string& local_ip = "", uint16_t local_port = 0);

    void setConnectState(TcpSocket::ConnectState state);

    TcpSocket::ConnectState getConnectState() const;

    bool setKeepalive(bool keepalive = true, int interval = TCP_KEEPALIVE_INTERVAL, int idle = TCP_KEEPALIVE_TIME, int times = TCP_KEEPALIVE_PROBE_TIMES);

    bool setNoDelay(bool nodelay);

    int32_t send(const char* data, int32_t length);

    int32_t recv(char* buffer, int32_t length);

    int32_t writeV(const struct iovec* vector, int32_t count);
    
    std::string getRemoteIp() const { return remote_ip_; };

    uint16_t getRemotePort() const { return remote_port_; };

    bool getRemoteAddress(std::string &ip, uint16_t &port);

private:
    ConnectState conncect_state_;
    std::string remote_ip_;
    uint32_t remote_port_;
};
}