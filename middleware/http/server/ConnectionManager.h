/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ConnectionManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 14:06:07
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <map>
#include <mutex>
//#include "Connection.h"

class Connection;
class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();
    void addNewConnection(std::shared_ptr<Connection> &new_connection);
    void delConnection(int32_t fd);
    void clear();
private:
    std::map<int32_t, std::shared_ptr<Connection>> connection_map_;
    std::mutex  connection_map_mutex_;
};