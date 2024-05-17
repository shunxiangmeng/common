/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Thread.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-23 21:15:49
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/thread/Thread.h"
#include "infra/include/Logger.h"
#if defined(_WIN32)
#include <Windows.h>
#else
#include<unistd.h>
#endif

namespace infra {

Thread::Thread(Priority priority, const std::string &name, bool affinity) :
    priority_(priority), name_(name), running_(false), tid_(-1) {
}

Thread::~Thread() {
    stop();
}

bool Thread::start() {
    running_ = true;
    //thread_ = std::make_shared<std::thread>([this]() { run(); });
    thread_ = std::thread([this]() { run(); });
    return true;
}

void Thread::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool Thread::setPriority(Priority priority, std::thread::native_handle_type threadId) {
#if defined(_WIN32)
    static int Priorities[] = {THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};
    if (priority != PRIORITY_NORMAL && SetThreadPriority(GetCurrentThread(), Priorities[priority]) == 0) {
        return false;
    }
    return true;
#else
    static int Min = sched_get_priority_min(SCHED_OTHER);
    if (Min == -1) {
        return false;
    }
    static int Max = sched_get_priority_max(SCHED_OTHER);
    if (Max == -1) {
        return false;
    }
    static int Priorities[] = {Min, Min + (Max - Min) / 4, Min + (Max - Min) / 2, Min + (Max - Min) * 3 / 4, Max};

    if (threadId == 0) {
        threadId = pthread_self();
    }
    struct sched_param params;
    params.sched_priority = Priorities[priority];
    return pthread_setschedparam(threadId, SCHED_OTHER, &params) == 0;
#endif
}

void Thread::setTid(int32_t tid) {
    tid_ = tid;
}

int32_t Thread::tid() const {
    return tid_;
}

}