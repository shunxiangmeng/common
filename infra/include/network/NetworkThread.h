/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  NetworkTHread.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:03:55
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <mutex>
#include "../thread/Thread.h"
#include "SocketHandler.h"
#include "Pipe.h"

#if defined(__linux__) || defined(__linux)
#define HAS_EPOLL 1
#define EPOLL_SIZE 1024
#endif //__linux__

namespace infra {

class NetworkThread : public Thread, public TaskExecutor, public ThreadLoadCounter {
public:
    NetworkThread(Priority priority = PRIORITY_HIGHEST, const std::string &name = "netthread", bool affinity = true);
    virtual ~NetworkThread();

    bool start();

    virtual bool postTask(Task &&task) override;
    virtual int64_t postDelayedTask(Task &&task, int64_t delay_time_ms) override;

    bool addEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> &handler);
    bool modifyEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> &handler);
    bool delEvent(int32_t fd, std::shared_ptr<SocketHandler> &handler);

private:
    virtual void run() override;
    virtual void wakeUp() override;
    
private:
    struct EventRecord {
        int32_t fd;
        SocketHandler::EventType event;
        std::shared_ptr<SocketHandler> handler;
    };

#if defined(HAS_EPOLL)
    //epoll相关
    int32_t epoll_fd_ = -1;
#endif
    Pipe pipe_;
    std::map<int32_t, std::shared_ptr<EventRecord>> event_record_map_;
    std::mutex event_record_map_mutex_;
};

}