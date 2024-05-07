/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TransportUdp.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 16:12:56
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "Transport.h"
#include "infra/include/Signal.h"
#include "infra/include/network/UdpSocket.h"

class TransportUdp : public Transport {
public:

    TransportUdp(std::shared_ptr<infra::Socket> &sock, bool needClose);

    virtual ~TransportUdp();

    void destroy();

    virtual bool startReceive();

    virtual bool stopReceive();

    virtual int32_t send(const char *data, int32_t dataLen);

    virtual int32_t setChannelId(int channelId);

    virtual int32_t setOption(TransportOpt optName, void *optValue, int len);

private:

    virtual int32_t onRead(int32_t fd);

    virtual int32_t onException(int32_t fd);

private:
    bool                 start_;
    uint32_t             mSSRC;
    std::shared_ptr<infra::UdpSocket>  mSock;
    std::mutex           mMutex;  /** 保护释放 */

#define UDP_RBUF_LEN  2048

    char  recv_buffer[UDP_RBUF_LEN];
    int32_t recv_buffer_data_size;
};
