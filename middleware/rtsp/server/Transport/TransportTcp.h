/************************************************
 * Copyright(c) 2021 uni-ubi
 * 
 * Project:    Rtsp
 * FileName:   TransportTcp.h
 * Author:     jinlun
 * Email:      jinlun@uni-ubi.com
 * Version:    V1.0.0
 * Date:       2021-02-02
 * Description: 
 * Others:
 *************************************************/
#pragma once
#include <string>
#include <memory>
#include "Transport.h"
#include "infra/include/Signal.h"
#include "infra/include/network/TcpSocket.h"

class TransportTcp : public Transport {
public:

    TransportTcp(int32_t fd, bool needClose);

    TransportTcp(std::shared_ptr<infra::Socket> &sock, bool needClose);

    virtual ~TransportTcp();

    void destroy();

    virtual bool startReceive();

    virtual bool stopReceive();

    virtual int32_t send(const char *data, int32_t dataLen);

private:

    virtual int32_t onRead(int32_t fd);

    virtual int32_t onException(int32_t fd);
private:

#define CMD_SBUF_LEN  4096
#define CMD_RBUF_LEN  4096

    bool                                 start_;
    std::shared_ptr<infra::TcpSocket>    sock_;
    char                                 mCmdSendBuffer[CMD_SBUF_LEN];   /**命令发送buf*/
    char                                 mRecvBuffer[CMD_RBUF_LEN];
    int32_t                              mCmdSendBufferLen;
    int32_t                              mRecvBufferLen;
    int32_t                              mRecvBufferDataLen;            /** 已接收数据长度 */
    std::recursive_mutex                 mutex_;
};
