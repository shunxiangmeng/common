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
#include "RtspService.h"
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/Logger.h"

IRtspService* IRtspService::instance() {
    static std::shared_ptr<RtspService> s_rtsp = std::make_shared<RtspService>();
    return s_rtsp.get();
}

RtspService::RtspService() : port_(-1) {
    session_manager_ = std::make_shared<RtspSessionManager>();
}

RtspService::~RtspService() {
}

bool RtspService::start(int32_t port) {
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

bool RtspService::stop() {
    warnf("rtsp service stop\n");
    infra::NetworkThreadPool::instance()->delSocketEvent(acceptor_.getHandle(), shared_from_this());
    acceptor_.close();
    return true;
}

bool RtspService::restart(int32_t port) {
    return false;
}

int32_t RtspService::onRead(int32_t fd) {
    std::shared_ptr<infra::TcpSocket> newsocket = acceptor_.accept();
    if (newsocket && session_manager_) {
        session_manager_->addNewConnect(newsocket);
    }
    return 0;
}

int32_t RtspService::OnException(int32_t fd) {
    return -1;
}

void RtspService::setAuthority(bool authority) {
    session_manager_->setAuthority(authority);
}

void RtspService::dumpsessions() {
    if (!session_manager_) {
        return;
    }
    session_manager_->dumpSessions();
}
