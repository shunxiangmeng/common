/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TcpIO.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 19:08:23
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include "infra/include/network/TcpSocket.h"
#include "infra/include/network/SocketHandler.h"

namespace infra {
using AsyncReadCallback = std::function<void(int32_t)>;

class TcpIO : public TcpSocket, public SocketHandler {
public:
    TcpIO();
    virtual ~TcpIO();

    bool asyncReadSome(char* buffer, int32_t size, AsyncReadCallback callback);
    bool stop();
private:
    virtual int32_t onRead(int32_t fd) override;
    virtual int32_t onException(int32_t fd) override;

    AsyncReadCallback read_callback_;
};
}