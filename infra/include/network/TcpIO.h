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
#include <mutex>
#include <functional>
#include "infra/include/network/TcpSocket.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/Buffer.h"

namespace infra {
using AsyncReadCallback = std::function<void(infra::Buffer&)>;
using ExceptionCallback = std::function<void(int32_t)>;

class TcpIO : public TcpSocket, public SocketHandler {
public:
    TcpIO();
    virtual ~TcpIO();

    bool setCallback(AsyncReadCallback callback, ExceptionCallback exception_callback);
    bool stop();
private:
    virtual int32_t onRead(int32_t fd) override;
    virtual int32_t onException(int32_t fd) override;

    bool ensureRead(int32_t &readed_size);

    AsyncReadCallback read_callback_;
    ExceptionCallback exception_callback_;

    infra::Buffer buffer_;
    std::recursive_mutex mutex_;
    bool attach_event_ = false;
};
}