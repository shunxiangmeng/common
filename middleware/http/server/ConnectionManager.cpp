/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ConnectionManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 14:13:09
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "ConnectionManager.h"
#include "infra/include/Logger.h"
#include "Connection.h"

ConnectionManager::ConnectionManager() {
}

ConnectionManager::~ConnectionManager() {
}

void ConnectionManager::addNewConnection(std::shared_ptr<Connection> &new_connection) {
    int32_t fd = new_connection->handle();
    std::lock_guard<std::mutex> guard(connection_map_mutex_);
    auto it = connection_map_.find(fd);
    if (it != connection_map_.end()) {
        errorf("connection fd %d already in manager\n", fd);
        return;
    }
    connection_map_[fd] = new_connection;
    infof("httpserver connection count:%d\n", connection_map_.size());
}

void ConnectionManager::delConnection(int32_t fd) {
    std::lock_guard<std::mutex> guard(connection_map_mutex_);
    connection_map_.erase(fd);
}

void ConnectionManager::clear() {
    std::lock_guard<std::mutex> guard(connection_map_mutex_);
    connection_map_.clear();
}
