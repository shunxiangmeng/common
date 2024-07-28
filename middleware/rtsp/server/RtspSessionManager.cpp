/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSessionManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 22:27:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "RtspSessionManager.h"
#include "infra/include/Logger.h"
#include "RtspSession.h"

RtspSessionManager::RtspSessionManager() : max_connect_(4), session_count_(0) { 
}

RtspSessionManager::~RtspSessionManager() {
}

void RtspSessionManager::addNewConnect(std::shared_ptr<infra::TcpSocket> &newsock) {
    tracef("RtspSessionManager::addNewConnect+++ fd:%d\n", newsock->getHandle());
    if (session_count_ > max_connect_) {
        errorf("addNewConnect error, connection count %d\n", session_count_);
        return;
    }

    std::shared_ptr<ConnectInfo> new_connect = std::shared_ptr<ConnectInfo>(new ConnectInfo());
    new_connect->sock = newsock;

    std::lock_guard<std::mutex> guard(connect_queue_mutex_);
    connect_queue_[new_connect->sock->getHandle()] = new_connect;

    auto event_type = infra::SocketHandler::EventType(infra::SocketHandler::EventType::read | infra::SocketHandler::EventType::except);
    if (!infra::NetworkThreadPool::instance()->addSocketEvent(newsock->getHandle(), event_type, shared_from_this())) {
        errorf("addSocketEvent error\n");
        connect_queue_.erase(new_connect->sock->getHandle());
        return;
    }
}

int32_t RtspSessionManager::onRead(int32_t fd) {
    connect_queue_mutex_.lock();
    auto it = connect_queue_.find(fd);
    if (it == connect_queue_.end()){
        errorf("invalid socket fd:%d\n", fd);
        connect_queue_mutex_.unlock();
        return 0;
    }
    auto &connect = it->second;
    connect_queue_mutex_.unlock();

    std::shared_ptr<infra::TcpSocket> &sock = connect->sock;
    char *request = connect->request;
    int32_t request_len = connect->request_len;
    int32_t ret = sock->recv(request, ConnectInfo::MaxPrevBufferSize - request_len);

    if (ret < 0) {
        errorf("error, close sock\n");
        infra::NetworkThreadPool::instance()->delSocketEvent(sock->getHandle(), shared_from_this());
        //sock->close();
        connect_queue_mutex_.lock();
        connect_queue_.erase(it);
        connect_queue_mutex_.unlock();
        return -1;
    } else if (ret > 0) {
        infra::NetworkThreadPool::instance()->delSocketEvent(sock->getHandle(), shared_from_this()); ///manager不再接收数据
        connect->request_len += ret;
        if (strstr(request, "\r\n\r\n") == nullptr) {
            return 0;
        }

        //only for debug
        std::string ip;
        uint16_t port;
        sock->getRemoteAddress(ip, port);
        infof("new rtsp connect fd:%d from %s:%d\n%s\n", fd, ip.data(), port, request);
        ///允许直接发送DESCRIBE进行连接
        if ((strstr(request, "OPTIONS ") != nullptr) || (strstr(request, "DESCRIBE ") != nullptr)) {
            std::shared_ptr<RtspSession> rtsp = std::make_shared<RtspSession>(this);
            session_list_mutex_.lock();
            session_list_.push_back(rtsp);
            session_list_mutex_.unlock();

            if (!rtsp->init(sock, request, ret)) {
                session_list_mutex_.lock();
                session_list_.remove(rtsp);
                session_list_mutex_.unlock();

                connect_queue_mutex_.lock();
                connect_queue_.erase(it);
                connect_queue_mutex_.unlock();
                return -1;
            }
        } else if ((strstr(request, "GET /") != nullptr) && (strstr(request, "Upgrade: websocket") != nullptr)) {
            ///rtsp over websocket
            warnf("not support rtsp over websocket\n");
            connect_queue_mutex_.lock();
            connect_queue_.erase(it);
            connect_queue_mutex_.unlock();
            return -1;
        } else if (strstr(request, "GET /") != nullptr){
            ///rtsp over http
            warnf("not support rtsp over http\n");
            connect_queue_mutex_.lock();
            connect_queue_.erase(it);
            connect_queue_mutex_.unlock();
            return -1;
        }

        ///连接管理完成，移除创建连接队列
        connect_queue_mutex_.lock();
        connect_queue_.erase(it);
        connect_queue_mutex_.unlock();
    }
    return 0;
}

int32_t RtspSessionManager::onException(int32_t fd) {
    warnf("session manager exception fd:%d\n", fd);
    connect_queue_mutex_.lock();
    auto it = connect_queue_.find(fd);
    if (it == connect_queue_.end()){
        errorf("invalid socket fd:%d\n", fd);
        connect_queue_mutex_.unlock();
        return 0;
    }

    std::shared_ptr<ConnectInfo> &connect = it->second;
    connect_queue_mutex_.unlock();
    std::shared_ptr<infra::TcpSocket> &sock = connect->sock;

    infra::NetworkThreadPool::instance()->delSocketEvent(sock->getHandle(), shared_from_this());
    connect_queue_mutex_.lock();
    connect_queue_.erase(it);
    connect_queue_mutex_.unlock();
    return 0;
}

void RtspSessionManager::dumpSessions() {
}

int32_t RtspSessionManager::remove(std::shared_ptr<RtspSession> session) {
    session_list_mutex_.lock();
    session_list_.remove(session);
    session_list_mutex_.unlock();
    return 0;
}

void RtspSessionManager::setAuthority(bool authority) {
    is_authority_ = authority;
}

bool RtspSessionManager::isAuthority() {
    return is_authority_;
}
