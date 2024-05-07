/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  NetworkThreadPool.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:48:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include <memory>
#include "NetworkThread.h"
#include "SocketHandler.h"

namespace infra {

class NetworkThreadPool {
    NetworkThreadPool() = default;
    ~NetworkThreadPool() = default;

public:
    static NetworkThreadPool* instance();

    bool init(int32_t thread_num = 0);

    bool addSocketEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> handler);

    bool modifySocketEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> handler);

    bool delSocketEvent(int32_t fd, std::shared_ptr<SocketHandler> handler);

private:
    friend class Timer;
    std::shared_ptr<NetworkThread> getThread();

private:
    std::vector<std::shared_ptr<NetworkThread>> network_threads_;
    std::mutex network_threads_mutex_;

};

}