/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UdpSocket.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 10:51:10
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "infra/include/network/Socket.h"
#include "infra/include/network/Defines.h"

namespace infra {
class UdpSocket : public Socket {
public:
    UdpSocket();

    virtual ~UdpSocket();

    /**
     * @brief udp打开本地端口，接收对端的数据
     * @param[in] ip
     * @param[in] port
     * @param[in] reuseaddr
     * @return 
     */
    int32_t open(const std::string& ip, uint16_t port, bool reuseaddr = true);

    int32_t send(const char *buffer, int32_t len, struct sockaddr_in &remote);

    int32_t recv(char *buffer, int32_t len, struct sockaddr_in *remote = nullptr);

    virtual uint16_t getLocalPort() override;

    int32_t setRemoteAddr(struct sockaddr_in &remote);

    /**
     * @brief 在调用了 setRemoteAddr 之后可以直接调用此函数发送数据
     * @param[in] buffer
     * @param[in] len
     * @return 
     */
    int32_t send(const char *buffer, int32_t len);

private:
    uint16_t local_port_;

};
} // namespace infra
