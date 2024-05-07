/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TransportUdp.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 16:13:46
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "TransportUdp.h"
#include "infra/include/Logger.h"
#include "infra/include/network/NetworkThreadPool.h"

TransportUdp::TransportUdp(std::shared_ptr<infra::Socket> &sock, bool need_close) : start_(false) {
    tracef("create udp transport fd:%d\n", sock->getHandle());
    mSock = std::dynamic_pointer_cast<infra::UdpSocket>(sock);
    mChannelId = -1;
    mSSRC = 0x00000000;
    mNeedCloseSock = need_close;
}

TransportUdp::~TransportUdp() {
}

void TransportUdp::destroy() {
}

bool TransportUdp::startReceive() {
    infof("TransportUdp::startReceive+++\n");
    if (start_) {
        return true;
    }
    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(mSock->getHandle(), event_type, shared_from_this())) {
        errorf("addSocketEvent error\n");
        return false;
    }
    start_ = true;
    return true;
}

bool TransportUdp::stopReceive() {
    if (!start_) {
        return true;
    }
    if (!infra::NetworkThreadPool::instance()->delSocketEvent(mSock->getHandle(), shared_from_this())) {
        errorf("delSocketEvent error\n");
    }
    start_ = false;
    return false;
}

int32_t TransportUdp::setChannelId(int channelId) {
    mChannelId = channelId;
    return 0;
}

int32_t TransportUdp::setOption(TransportOpt optName, void *optValue, int len) {
    if (optValue == nullptr) {
        errorf("optValue is nullptr\n");
        return -1;
    }

    int32_t ret = -1;
    switch (optName) {
        case transportOptSndBuffer: {
            uint32_t curlen = 0; 
            socklen_t socklen = sizeof(curlen);
            int32_t fd = mSock->getHandle();
            int32_t retget = ::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&curlen, &socklen); 
            tracef("transport send buffer opt, old buf:%u, ret:%d, send buf:%u \n", curlen, retget, *(uint32_t*)optValue);
            ret = ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF , (char *)optValue, len);
            retget = ::getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&curlen, &socklen);
            tracef("transport send buffer opt,current buf:%u, ret:%d \n", curlen, retget);
            break;
        }

        case transportOptRecvBuffer: {
            uint32_t curlen = 0;            
            socklen_t socklen = sizeof(curlen);
            int32_t fd = mSock->getHandle();
            int32_t retget = ::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&curlen, &socklen);            
            tracef("transport recv buffer opt, old buf:%u, ret:%d, recv buf:%u \n", curlen, retget, *(uint32_t*)optValue);
            ret = ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)optValue, len );
            retget = ::getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&curlen, &socklen);
            tracef("transport recv buffer opt, current buf:%u,ret:%d \n", curlen, retget);
            break;
        }

        case transportOptRemoteAddr: {
            struct sockaddr_in* addr = (struct sockaddr_in*)optValue;
            ret = mSock->setRemoteAddr(*addr);
            break;
        }
        case transportOptTTL:
            errorf("not impl transportOptTTL\n");
            // todo ret = mSock.setMulticastTTL(*(uint8_t*)optValue);
            break;    
        default:
            errorf("set option not Implemented, optName=%d.\n",optName);
            break;                         
    }

    return ret;
}

int32_t TransportUdp::send(const char *data, int32_t size) {
    if (data == nullptr) {
        return 0;
    }
    int32_t ret = mSock->send(data, size);
    if (ret < 0) {
        errorf("udp send error, ret:%d, size:%d\n", ret, size);
    }
    return ret;
}

int32_t TransportUdp::onRead(int32_t fd) {
    //tracef("udp fd %d onread\n", fd);
    if (fd != mSock->getHandle()) {
        return -1;
    }
    int32_t ret = mSock->recv(recv_buffer, sizeof(recv_buffer));
    if (ret < 0) {
        errorf("recv error, ret:%d\n", ret);
        if (exception_callback_) {
            exception_callback_(1);
        }
        return -1;
    } else if (ret == 0) {
        errorf("ret = 0\n");
        return 0;
    } else {
        if (mDataCallback) {
            mDataCallback(mChannelId, recv_buffer, ret);
        }
    }
    return 0;
}

int32_t TransportUdp::onException(int32_t socketFd) {
    if (socketFd != mSock->getHandle()) {
        return -1;
    }
    errorf("sock exception!!!\n");
    mMutex.lock();
    if (exception_callback_) {
        exception_callback_(socketFd);
    }
    mMutex.unlock();
    return 0;
}
