/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TcpIO.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 19:08:11
 * Description :  对TcpSocket的封装，简化了使用
 * Note        : 
 ************************************************************************/
#include "TcpIO.h"
#include "infra/include/network/NetworkThreadPool.h"

namespace infra {

TcpIO::TcpIO() {
}

TcpIO::~TcpIO() {
}

bool TcpIO::asyncReadSome(char* buffer, int32_t size, AsyncReadCallback callback) {
    SocketHandler::EventType event_type = SocketHandler::EventType(SocketHandler::read | SocketHandler::except);
    if (!NetworkThreadPool::instance()->addSocketEvent(getHandle(), event_type, shared_from_this())) {
        this->close();
        return false;
    }
    read_callback_ = callback;
    return true;
}

bool TcpIO::stop() {
    NetworkThreadPool::instance()->delSocketEvent(getHandle(), shared_from_this());
    this->close();
    return true;
}

int32_t TcpIO::onRead(int32_t fd) {
    //std::shared_ptr<TcpSocket> newsocket = this->accept();
    //read_callback_(newsocket);
    return 0;
}

int32_t TcpIO::onException(int32_t fd) {
    //errorf("acceptor exception %d\n", fd);
    return 0;
}

} // namespace infra
