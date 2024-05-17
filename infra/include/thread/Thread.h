/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Thread.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:12:58
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <thread>
#include "TaskExecutor.h"
#include "ThreadLoadCounter.h"

namespace infra {

class Thread {
public:
    enum Priority {
        PRIORITY_LOWEST = 0,
        PRIORITY_LOW,
        PRIORITY_NORMAL,
        PRIORITY_HIGH,
        PRIORITY_HIGHEST
    };

    Thread(Priority priority = PRIORITY_HIGHEST, const std::string &name = "thread", bool affinity = true);
    virtual ~Thread();

    bool start();
    void stop();

    bool running() const {
        return running_;
    }

    std::thread::id id() const {
        return thread_.get_id();
    }

    int32_t tid() const;

protected:
    bool setPriority(Priority priority = PRIORITY_NORMAL, std::thread::native_handle_type threadId = 0);
    void setTid(int32_t tid);

private:
    virtual void run() = 0;
    virtual void wakeUp() {};

protected:
    Priority priority_;
    std::string name_;
    bool running_;

private:
    bool affinity_;
    std::thread thread_;
    int32_t tid_;
};

}