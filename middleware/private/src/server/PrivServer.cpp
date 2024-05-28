/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:35:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivServer.h"
#include "PrivEventManager.h"
#include "infra/include/Logger.h"
#include "infra/include/network/NetworkThreadPool.h"

IPrivServer* IPrivServer::instance() {
    static std::shared_ptr<PrivServer> s_server = std::make_shared<PrivServer>();
    return s_server.get();
}

PrivServer::PrivServer() : port_(0), session_manager_(nullptr) {
    session_manager_ = std::make_shared<PrivSessionManager>();
}

PrivServer::~PrivServer() {
}

bool PrivServer::start(uint16_t port){
    if (!acceptor_.listen("0.0.0.0", port)) {
        errorf("RtspService listen port %d error\n", port);
        return false;
    }
    infra::SocketHandler::EventType event_type = infra::SocketHandler::EventType(infra::SocketHandler::read | infra::SocketHandler::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(acceptor_.getHandle(), event_type, shared_from_this())) {
        acceptor_.close();
        return false;
    }
    infof("start privateServer on port %d\n", port);
    return true;
}

bool PrivServer::restart(uint16_t port) {
    return false;
}

int32_t PrivServer::onRead(int32_t fd) {
    std::shared_ptr<infra::TcpSocket> newsocket = acceptor_.accept();
    if (newsocket && session_manager_) {
        session_manager_->addNewConnect(newsocket);
    }
    return 0;
}

int32_t PrivServer::onException(int32_t fd) {
    errorf("priv_server socket exception\n");
    return -1;
}

bool PrivServer::stop() {
    warnf("stop privateService\n");
    infra::NetworkThreadPool::instance()->delSocketEvent(acceptor_.getHandle(), shared_from_this());
    acceptor_.close();
    return true;
}

bool PrivServer::addSubscribeEvent(const char* name) {
    return PrivEventManager::instance()->addSubscribeEvent(name);
}

bool PrivServer::sendEvent(const char* name, const Json::Value &event) {
    if (session_manager_) {
        if (session_manager_->sendEvent(name, event) == false) {
            errorf("session_manager_->sendEvent error\n");
            return false;
        }
    } else {
        errorf("session_manager_ is nullptr\n");
        return false;
    }
    return true;
}
