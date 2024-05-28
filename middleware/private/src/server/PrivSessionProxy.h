/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSessionProxy.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:31:29
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "infra/include/network/TcpSocket.h"

typedef std::shared_ptr<infra::TcpSocket> NetSocketPtr;

class IPrivSessionProxy {
public:

    /**
     * @brief 接收数据
     * @param sock
     * @param buffer
     * @param bufferLen
     * @return
     */
    virtual int32_t recvData(NetSocketPtr &sock, char *buffer, uint32_t bufferLen) = 0;
    /**
     * @brief 发送数据
     * @param buffer
     * @param size
     * @return
     */
    virtual int32_t sendData(const char* buffer, int32_t size) = 0;
};
