/************************************************
 * Copyright(c) 2021 uni-ubi
 * 
 * Project:    Rtsp
 * FileName:   TransportTcp.cpp
 * Author:     jinlun
 * Email:      jinlun@uni-ubi.com
 * Version:    V1.0.0
 * Date:       2021-02-02
 * Description: 
 * Others:
 *************************************************/
#include "TransportTcp.h"
#include "infra/include/Logger.h"
#include "infra/include/network/NetworkThreadPool.h"

TransportTcp::TransportTcp(int32_t fd, bool need_close):
                start_(false), mRecvBufferLen(CMD_RBUF_LEN), mRecvBufferDataLen(0) {

    mNeedCloseSock = need_close;
    sock_ = std::shared_ptr<infra::TcpSocket>(new infra::TcpSocket(fd));
    if (sock_ == nullptr) {
        errorf("sock_.get() is nullptr\n");
    }
}

TransportTcp::TransportTcp(std::shared_ptr<infra::Socket> &sock, bool needClose) : 
    start_(false), mRecvBufferLen(CMD_RBUF_LEN), mRecvBufferDataLen(0) {
    sock_ = std::dynamic_pointer_cast<infra::TcpSocket>(sock);
    mNeedCloseSock = needClose;
}

TransportTcp::~TransportTcp() {
}

void TransportTcp::destroy() {
}

bool TransportTcp::startReceive() {
    if (start_) {
        return true;
    }
    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(sock_->getHandle(), event_type, shared_from_this())) {
        errorf("addSocketEvent error\n");
        return false;
    }

    int32_t send_buffer = sock_->getSendBuffer();
    infof("transport send buffer:%d\n", send_buffer);
    sock_->setSendBuffer(1024 * 1024);
    send_buffer = sock_->getSendBuffer();
    infof("transport send buffer:%d\n", send_buffer);

    start_ = true;
    return true;
}

bool TransportTcp::stopReceive() {
    if (!start_) {
        return true;
    }
    if (!infra::NetworkThreadPool::instance()->delSocketEvent(sock_->getHandle(), shared_from_this())) {
        errorf("delSocketEvent error\n");
    }    
    mutex_.lock();  //wait for mDataCallback
    mDataCallback = DataProc();
    mutex_.unlock();
    start_ = false;
    return true;
}

int32_t TransportTcp::send(const char *data, int32_t dataLen) {
    if (data == nullptr){
        return 0;
    }
    return sock_->send(data, dataLen);
}

int32_t TransportTcp::onRead(int32_t socket_fd) {
    if (socket_fd != sock_->getHandle()) {
        return -1;
    }
    mutex_.lock();
    int32_t ret = sock_->recv(mRecvBuffer + mRecvBufferDataLen, mRecvBufferLen - mRecvBufferDataLen);
    if (ret < 0) {
        errorf("disconnect, ret:%d\n", ret);
        if (exception_callback_) {
            exception_callback_(1);
        }

        mutex_.unlock();
        return -1;
    } else if (ret == 0) {
        tracef("recv ret = 0\n");
        mutex_.unlock();
        return 0;
    }

    int32_t used = 0;
    mRecvBufferDataLen += ret;
    mRecvBuffer[mRecvBufferDataLen] = 0;
    if (mDataCallback) {
        used = mDataCallback(mChannelId, mRecvBuffer, mRecvBufferDataLen);
        if (used > 0){
            mRecvBufferDataLen -= used;
            if (mRecvBufferDataLen > 0) {
                warnf("left(%d) > 0\n", mRecvBufferDataLen);
                memcpy(mRecvBuffer, mRecvBuffer + used, mRecvBufferDataLen);
            }
        }
    }
    mutex_.unlock();
    return 0;
}

int32_t TransportTcp::onException(int32_t fd) {
    if (fd != sock_->getHandle()) {
        return -1;
    }
    errorf("sock exception, fd:%d\n", fd);
    mutex_.lock();
    if (exception_callback_) {
        exception_callback_(fd);
    }
    mutex_.unlock();
    return 0;
}
