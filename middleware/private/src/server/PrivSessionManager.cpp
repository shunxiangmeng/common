/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSessionManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:53:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string.h>
#include <algorithm>
#include "PrivSessionManager.h"
#include "WebSocket/PrivWebsocketSession.h"
#include "infra/include/Timestamp.h"

PrivSessionManager::PrivSessionManager(RPCServer* rpc_server) : max_connect_(16), mCloseSessionTimer(0), rpc_server_(rpc_server) {
}

PrivSessionManager::~PrivSessionManager() {
}

void PrivSessionManager::addNewConnect(std::shared_ptr<infra::TcpSocket> &newsock){
    tracef("PrivSessionManager::addNewConnect+++ fd:%d\n", newsock->getHandle());
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

std::shared_ptr<PrivSession> PrivSessionManager::addNewSession(const char* name, uint32_t sessionId){
    uint32_t id = 0;
    std::lock_guard<std::mutex> guard(mMainSessionMapMutex);
    if (sessionId != 0){
        id = sessionId;
        if (mMainSessionMap.find(id) != mMainSessionMap.end()){
            errorf("session map has this sessionIs %u\n", id);
            return nullptr;
        }
    } else{
        do {
            id = uint32_t(infra::getCurrentTimeUs());
            id &= 0x00FFFFFF;    ///最高位用来做别的定义
            if (mMainSessionMap.find(id) == mMainSessionMap.end()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (true);
    }

    auto session = std::make_shared<PrivSession>(this, name, id, rpc_server_);
    mMainSessionMap[id] = session;
    return session;
}

int32_t PrivSessionManager::remove(uint32_t session_id) {
    session_map_mutex_.lock();
    infof("session_map size:%d, id:%d\n", session_map_.size(), session_id);
    session_map_.erase(session_id);
    session_map_mutex_.unlock();
    return 0;
}

int32_t PrivSessionManager::onRead(int32_t fd) {
    //warnf("new connection fd:%d\n", fd);
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
        std::string ip;
        uint16_t port;
        sock->getRemoteAddress(ip, port);
        
        ///私有协议连接
        if (strstr(request, "@@@@") || strstr(request, "****")) {
            infof("new private connect fd:%d from %s:%d\n%s\n", fd, ip.data(), port, request);
            std::shared_ptr<PrivSession> session;
            uint32_t id = 0;
            {
                std::lock_guard<std::mutex> guard(session_map_mutex_);
                do {
                    id = uint32_t(infra::getCurrentTimeUs());
                    id &= 0x0FFFFFFF;    ///最高4位用来做别的定义
                    if (session_map_.find(id) == session_map_.end()) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (true);
                session = std::make_shared<PrivSession>(this, "mainSession", id, rpc_server_);
                session_map_[id] = session;
            }
            (void)session->initial(sock, request, ret);

        } else if (strstr(request, "\r\n\r\n") != nullptr) {
            if ((strstr(request, "GET /") != nullptr) && (strstr(request, "Upgrade: websocket") != nullptr)) {
                infof("new private over websocket connect fd:%d from %s:%d\n%s\n", fd, ip.data(), port, request);
                /// priv over websocket
                std::shared_ptr<PrivWebsocketSession> session;
                uint32_t id = 0;    
                {
                    std::lock_guard<std::mutex> guard(session_map_mutex_);
                    do {
                        id = uint32_t(infra::getCurrentTimeUs());
                        id &= 0x00FFFFFF;    ///最高8位用来做别的定义
                        if (session_map_.find(id) == session_map_.end()){
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }  while (true);
                    session = std::make_shared<PrivWebsocketSession>(this, id, rpc_server_);
                    session_map_[id] = session;
                    infof("new websocket session id:%d\n", id);
                }
                session->initial(sock, request, ret);
            }

            // todo priv over http
        } else {
            errorf("bad connect with priv\n");
        }

        connect_queue_mutex_.lock();
        connect_queue_.erase(it);
        connect_queue_mutex_.unlock();
    }
    return 0;
}

int32_t PrivSessionManager::onException(int32_t fd) {
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

std::shared_ptr<PrivSession> PrivSessionManager::getSessionBySessionId(uint32_t session_id){
    std::lock_guard<std::mutex> guard(session_map_mutex_);
    auto it = session_map_.find(session_id);
    if (it == session_map_.end()) {
        return nullptr;
    }
    return it->second;
}

//给每个session都发送事件
bool PrivSessionManager::sendEvent(const char* name, const Json::Value &event) {
    std::lock_guard<std::mutex> guard(session_map_mutex_);
    for (auto &it : session_map_) {
        if (it.second) {
            it.second->sendEvent(name, event);
        } else {
            errorf("sessionId:%u it->second is nullptr\n", it.first);
            return false;
        }
    }
    return true;
}

bool PrivSessionManager::sendEvent(const char* name, const std::string &event) {
    std::lock_guard<std::mutex> guard(session_map_mutex_);
    for (auto &it : session_map_) {
        if (it.second) {
            it.second->sendEvent(name, event);
        } else {
            errorf("sessionId:%u it->second is nullptr\n", it.first);
            return false;
        }
    }
    return true;
}