/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Timer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-24 20:20:24
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/Timer.h"
#include "infra/include/Logger.h"

namespace infra {

Timer::Timer(std::string name) : task_id_(-1), running_(false), name_(name) {
}

Timer::~Timer() {
    stop();
}

bool Timer::start(int32_t period, const std::function<bool()> &proc) {
    running_ = true;
    run(period, proc);
    return true;
}

bool Timer::stop() {
    if (running_) {
        running_ = false;
        task_executor_->deleteDelayedTask(task_id_);
    }
    return true;
}

void Timer::run(int32_t period, const std::function<bool()> &proc) {
    task_executor_ = NetworkThreadPool::instance()->getThread();
    task_id_ = task_executor_->postDelayedTask([=]() {
        if (running_ && proc()) {
            run(period, proc);
        } else {
            running_ = false;
            infof("timer %p stop\n", this);
        }
    }, period);
}

}