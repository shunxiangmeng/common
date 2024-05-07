/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  NetworkThread.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:45:26
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/network/NetworkThread.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"
#include "infra/include/network/Defines.h"
#if defined(HAS_EPOLL)
#include <sys/epoll.h>
#define toEpoll(event)      (((event) & SocketHandler::read)  ? EPOLLIN : 0) \
                            | (((event) & SocketHandler::write) ? EPOLLOUT : 0) \
                            | (((event) & SocketHandler::except) ? (EPOLLHUP | EPOLLERR) : 0)
#endif

namespace infra {

NetworkThread::NetworkThread(Priority priority, const std::string &name, bool affinity) : Thread(priority, name, affinity),
    ThreadLoadCounter(32, 2 * 1000 * 1000) {
}

NetworkThread::~NetworkThread() {
#if defined(HAS_EPOLL)
#else
    pipe_.close();
#endif //HAS_EPOLL
}

bool NetworkThread::start() {
    if (!pipe_.open()) {
        errorf("network thread open pipe error\n");
        return false;
    }
#if defined(HAS_EPOLL)
    epoll_fd_ = epoll_create(EPOLL_SIZE);
    if (epoll_fd_ == -1) {
        errorf("epoll_create errno:%d\n", errno);
        return false;
    }

    int fd = pipe_.readFd();
    struct epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLEXCLUSIVE;
    ev.data.fd = fd;
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret != 0) {
        errorf("epoll add error ret:%d, errno:%d\n", ret, errno);
        return false;
    }
#endif //HAS_EPOLL

    return Thread::start();
}

void NetworkThread::wakeUp() {
    char buffer[1] = {'@'};
    pipe_.write(buffer, sizeof(buffer));
}

bool NetworkThread::addEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> &handler) {
#if defined(HAS_EPOLL)
    struct epoll_event ev = {0};
    ev.events = (toEpoll(event)) | EPOLLEXCLUSIVE;
    ev.data.fd = fd;
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret != 0) {
        errorf("epoll add error ret:%d, errno:%d\n", ret, errno);
        return false;
    }
#endif
    auto record = std::make_shared<EventRecord>();
    record->fd = fd;
    record->event = event;
    record->handler = handler;
    std::lock_guard<std::mutex> lock(event_record_map_mutex_);
    event_record_map_.emplace(fd, record);
    wakeUp();
    return true;
}

bool NetworkThread::modifyEvent(int32_t fd, SocketHandler::EventType event, std::shared_ptr<SocketHandler> &handler) {
    std::lock_guard<std::mutex> lock(event_record_map_mutex_);
    auto it = event_record_map_.find(fd);
    if (it == event_record_map_.end()) {
        errorf("modifyEvent not found fd:%d\n", fd);
        return false;
    } else {
        it->second->event = event;
    }
#if defined(HAS_EPOLL)
    struct epoll_event ev = { 0 };
    ev.events = toEpoll(event);
    ev.data.fd = fd;
    auto ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
    if (ret != 0) {
        errorf("epoll mod error ret:%d, errno:%d\n", ret, errno);
        return false;
    }
#else
    wakeUp();
#endif
    return true;
}

bool NetworkThread::delEvent(int32_t fd, std::shared_ptr<SocketHandler> &handler) {
    std::lock_guard<std::mutex> lock(event_record_map_mutex_);
    auto it = event_record_map_.find(fd);
    if (it == event_record_map_.end()) {
        return false;
    } else {
        event_record_map_.erase(fd);
    }
#if defined(HAS_EPOLL)
    int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    if (ret != 0) {
        errorf("epoll del fd:%d error ret:%d, errno:%d\n", fd, ret, errno);
        return false;
    }
#else
    wakeUp();
#endif
    return true;
}

void NetworkThread::run() {
    //infof("thread:%s start\n", name_.c_str());
    if (!setPriority(priority_)) {
        errorf("setPriority error\n");
    } else {
        debugf("setPriority %d success\n", int(priority_));
    }

    int64_t min_delay_ms = 0; 

#if defined(HAS_EPOLL)
    struct epoll_event events[EPOLL_SIZE] = {0};
    while (running_) {

        min_delay_ms = getDelayedTaskMinDelay();

        startSleep();
        int ret = epoll_wait(epoll_fd_, events, EPOLL_SIZE, min_delay_ms ? min_delay_ms : -1);
        sleepWakeUp();
        if (ret <= 0) {
            continue;
        }

        auto read_pipe = [&]() {
            char buffer[64] = {0};
            while (true) {
                int l = pipe_.read(buffer, sizeof(buffer));
                if (l >= sizeof(buffer)) {
                    continue;
                }
                //infof("network thread pipe read len:%d\n", l);
                break;
            }
        };

        if (ret == 1) {
            struct epoll_event &ee = events[0];
            int fd = ee.data.fd;
            if (fd == pipe_.readFd()) {
                read_pipe();
                continue;
            }
        }

        event_record_map_mutex_.lock();
        auto event_record_map_temp = event_record_map_;
        event_record_map_mutex_.unlock();

        for (int i = 0; i < ret; i++) {
            struct epoll_event &ee = events[i];
            int fd = ee.data.fd;
            if (fd == pipe_.readFd()) {
                read_pipe();
                continue;
            }

            auto it = event_record_map_temp.find(fd);
            if (it == event_record_map_temp.end()) {
                continue;
            }
            //tracef("epoll fd:%d events:0x%x, 0x%x, 0x%x, 0x%x\n", fd, ee.events, EPOLLOUT, EPOLLERR, EPOLLHUP);
            if (ee.events & EPOLLIN) {
                it->second->handler->onRead(fd);
            }
            if (ee.events & EPOLLOUT) {
                it->second->handler->onWrite(fd);
            }
            if (ee.events & EPOLLERR || ee.events & EPOLLHUP) {
                it->second->handler->onException(fd);

                if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) != 0) {
                    errorf("epoll del errno:%d\n", errno);
                }
                event_record_map_mutex_.lock();
                event_record_map_.erase(fd);
                event_record_map_mutex_.unlock();
            }
        }
    }

#else
    struct fd_set set_read, set_write, set_err;
    struct timeval tv;
    int32_t pipe_read_fd = pipe_.readFd();
    while (running_) {

        min_delay_ms = getDelayedTaskMinDelay();

        tv.tv_sec = (decltype(tv.tv_sec)) (min_delay_ms / 1000);
        tv.tv_usec = 1000 * (min_delay_ms % 1000);

        event_record_map_mutex_.lock();
        auto event_record_map_temp = event_record_map_;
        event_record_map_mutex_.unlock();

        FD_ZERO(&set_read);
        FD_ZERO(&set_write);
        FD_ZERO(&set_err);

        //添加管道读fd
        int32_t max_fd = pipe_read_fd;
        FD_SET(pipe_read_fd, &set_read);

        for (auto &it : event_record_map_temp) {
            if (it.first > max_fd) {
                max_fd = it.first;
            }
            if (it.second->event & SocketHandler::EventType::read) {
                FD_SET(it.first, &set_read);
            }
            if (it.second->event & SocketHandler::EventType::write) {
                FD_SET(it.first, &set_write);
            }
            if (it.second->event & SocketHandler::EventType::except) {
                FD_SET(it.first, &set_err);
            }
        }
        
        startSleep();
        int32_t ret = ::select(max_fd + 1, &set_read, &set_write, &set_err, min_delay_ms ? &tv : nullptr);
        sleepWakeUp();
        if (ret <= 0) {
            continue;
        }

        // 先检查管道并读完数据
        if (FD_ISSET(pipe_read_fd, &set_read)) {
            char buffer[64] = {0};
            while (true) {
                int l = pipe_.read(buffer, sizeof(buffer));
                if (l >= sizeof(buffer)) {
                    continue;
                }
                //infof("network thread pipe read len:%d\n", l);
                break;
            }
        }

        // 再检查IO事件
        for (auto &it : event_record_map_temp) {
            if (FD_ISSET(it.first, &set_read)) {
                it.second->handler->onRead(it.first);
            }
            if (FD_ISSET(it.first, &set_write)) {
                it.second->handler->onWrite(it.first);
            }
            if (FD_ISSET(it.first, &set_err)) {
                it.second->handler->onException(it.first);
            }
        }
    }
#endif
    infof("thread:%s exit!\n", name_.c_str());
}

bool NetworkThread::postTask(Task &&task) {
    errorf("network thread not impl postTask\n");
    return false;
}

int64_t NetworkThread::postDelayedTask(Task &&task, int64_t delay_time_ms) {
    int64_t task_id = TaskExecutor::postDelayedTask(std::move(task), delay_time_ms);
    wakeUp();
    return task_id;
}

}