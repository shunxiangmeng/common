/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  NetworkThreadPool.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:48:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/network/NetworkThreadPool.h"
#include "infra/include/Logger.h"

namespace infra {

NetworkThreadPool* NetworkThreadPool::instance() {
    static NetworkThreadPool s_network_threadpool;
    return &s_network_threadpool;
}

bool NetworkThreadPool::init(int32_t thread_num) {
    int32_t thread_num_real = thread_num;
    if (thread_num_real <= 0) {
        auto cpus = std::thread::hardware_concurrency();
        thread_num_real = cpus;
    }
    infof("network thread num %d, set %d\n", thread_num_real, thread_num);
    
    for (auto i = 0; i < thread_num_real; i++) {
        std::string name = "netthread_" + std::to_string(i);
        std::shared_ptr<NetworkThread> net_thread = std::make_shared<NetworkThread>(Thread::Priority::PRIORITY_HIGH, name);
        if (!net_thread->start()) {
            errorf("thread %s start error\n", name.data());
            return false;
        }
        network_threads_mutex_.lock();
        network_threads_.push_back(net_thread);
        network_threads_mutex_.unlock();
    }
    running_ = true;
    return true;
}

std::shared_ptr<NetworkThread> NetworkThreadPool::getThread() {
    if (network_threads_.size() == 0) {
        errorf("network_threads is empty\n");
        return nullptr;
    }
    auto min_load_thread = network_threads_[0];
    auto min_load = min_load_thread->load();
    std::lock_guard<std::mutex> lock(network_threads_mutex_);
    for (auto it : network_threads_) {
        auto load = it->load();
        if (load < min_load) {
            min_load = load;
            min_load_thread = it;
        }
    }
    return min_load_thread;
}

bool NetworkThreadPool::addSocketEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> handler) {
    if (!running_) {
        errorf("network_thread_pool is not running\n");
        return false;
    }
    auto executor = getThread();
    if (!executor) {
        return false;
    }
    return executor->addEvent(fd, event, handler);
}

bool NetworkThreadPool::modifySocketEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> handler) {
    std::lock_guard<std::mutex> lock(network_threads_mutex_);
    for (auto it : network_threads_) {
        if (it->modifyEvent(fd, event, handler)) {
            return true;
        }
    }
    return false;
}

bool NetworkThreadPool::delSocketEvent(int32_t fd, std::shared_ptr<SocketHandler> handler) {
    std::lock_guard<std::mutex> lock(network_threads_mutex_);
    for (auto it : network_threads_) {
        if (it->delEvent(fd, handler)) {
            return true;
        }
    }
    return false;
}

}