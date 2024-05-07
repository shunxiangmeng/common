/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspServiceImpl.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 22:04:30
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <memory>
#include "RtspServiceImpl.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/Logger.h"

RtspServiceImpl::RtspServiceImpl() : port_(-1) {
    session_manager_ = std::make_shared<RtspSessionManager>();
}

RtspServiceImpl::~RtspServiceImpl() {
}

bool RtspServiceImpl::start(int32_t port) {
    if (port < 0){
        errorf("port:%d error\n", port);
        return false;
    }

    if (!acceptor_.listen("0.0.0.0", port)) {
        errorf("RtspService listen port %d error\n", port);
        return false;
    }

    infra::SocketHandler::EventType event_type = infra::SocketHandler::EventType(infra::SocketHandler::read | infra::SocketHandler::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(acceptor_.getHandle(), event_type, shared_from_this())) {
        acceptor_.close();
        return false;
    }

    infof("start rtsp on port %d\n", port);
    return true;
}

bool RtspServiceImpl::stop() {
    warnf("rtsp service stop\n");
    infra::NetworkThreadPool::instance()->delSocketEvent(acceptor_.getHandle(), shared_from_this());
    acceptor_.close();
    return true;
}

bool RtspServiceImpl::restart(int32_t port) {
    return false;
}

int32_t RtspServiceImpl::onRead(int32_t fd) {
    std::shared_ptr<infra::TcpSocket> newsocket = acceptor_.accept();
    if (newsocket && session_manager_) {
        session_manager_->addNewConnect(newsocket);
    }
    return 0;
}

int32_t RtspServiceImpl::OnException(int32_t fd) {
    return -1;
}

void RtspServiceImpl::dumpsessions() {
    if (!session_manager_) {
        return;
    }
    session_manager_->dumpSessions();
}
