/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TcpIO.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 19:08:11
 * Description :  对TcpSocket的封装，简化了使用
 * Note        : 
 ************************************************************************/
#include "infra/include/network/TcpIO.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/Logger.h"

namespace infra {

TcpIO::TcpIO() : buffer_(4096) {
}

TcpIO::~TcpIO() {
    stop();
}

bool TcpIO::setCallback(AsyncReadCallback callback, ExceptionCallback exception_callback) {
    SocketHandler::EventType event_type = SocketHandler::EventType(SocketHandler::read | SocketHandler::except);
    if (!NetworkThreadPool::instance()->addSocketEvent(getHandle(), event_type, shared_from_this())) {
        this->close();
        return false;
    }
    read_callback_ = callback;
    exception_callback_ = exception_callback;
    return true;
}

bool TcpIO::stop() {
    NetworkThreadPool::instance()->delSocketEvent(getHandle(), shared_from_this());
    this->close();
    return true;
}

int32_t TcpIO::onRead(int32_t fd) {
    if (fd != this->getHandle()) {
        errorf("socketFd(%d) != getHandle(%d)\n", fd, this->getHandle());
        return -1;
    }
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    int32_t readed_size = 0;
    if (!ensureRead(readed_size)) {
        errorf("ensureRead failed, readed_size:%d\n", readed_size);
        if (exception_callback_) {
            exception_callback_(fd);
        }
        return -1;
    }
    if (read_callback_) {
        read_callback_(buffer_);
    }
    return 0;
}

int32_t TcpIO::onException(int32_t fd) {
    if (exception_callback_) {
        exception_callback_(fd);
        return 0;
    } else {
        return -1;
    }
}

bool TcpIO::ensureRead(int32_t &readed_size) {
    char *to_read_buffer = buffer_.data() + buffer_.size();
    int32_t to_read_length = buffer_.leftSize();
    int32_t ret = this->recv(to_read_buffer, to_read_length);
    if (ret < 0) {
        warnf("socket disconnect\n");
        return false;
    }
    if (ret >= 0 && ret < to_read_length) {
        buffer_.setSize(buffer_.size() + ret);
        readed_size += ret;
        return true;
    }
    buffer_.setSize(buffer_.size() + ret);
    readed_size += ret;

    // 2倍扩容
    int32_t to_expend_size = buffer_.capacity() * 2;
    if (to_expend_size > 2 * 1024 * 1024) {
        warnf("expand buffer size to %d error\n", to_expend_size);
        return false;
    }
    warnf("expand buffer size to %d\n", to_expend_size);
    if (!buffer_.ensureCapacity(to_expend_size)) {
        warnf("expand buffer size to %d error\n", to_expend_size);
        return false;
    }
    return ensureRead(readed_size);
}

} // namespace infra
