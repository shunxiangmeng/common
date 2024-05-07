/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ThreadTaskQueue.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 22:21:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/thread/WorkThread.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"

namespace infra {

WorkThread::WorkThread(Priority priority, const std::string &name, bool affinity) : Thread(priority, name, affinity),
    ThreadLoadCounter(32, 2 * 1000 * 1000) {
}

WorkThread::~WorkThread() {
}

void WorkThread::wakeUp() {
    semaphore_.post();
}

bool WorkThread::postTask(Task &&task) {
    std::lock_guard<decltype(task_queue_mutex_)> guard(task_queue_mutex_);
    task_queue_.push(std::move(task));
    wakeUp();
    return true;
}

int64_t WorkThread::postDelayedTask(Task &&task, int64_t delay_time_ms) {
    int64_t task_id = TaskExecutor::postDelayedTask(std::move(task), delay_time_ms);
    wakeUp();
    return task_id;
}

void WorkThread::run() {
    //infof("thread:%s start\n", name_.c_str());
    if (!setPriority(priority_)) {
        errorf("setPriority error\n");
    } else {
        debugf("setPriority %d success\n", int(priority_));
    }

    while (running_) {
        // 执行即时任务
        task_queue_mutex_.lock();
        if (!task_queue_.empty()) {
            size_t task_queue_size = task_queue_.size();
            auto task = task_queue_.front();
            task_queue_.pop();
            task_queue_mutex_.unlock();
            
            //infof("thread {} exec task, size:{}", index, (int)task_queue_size);
            task();  //执行任务
        } else {
            task_queue_mutex_.unlock();
        }
        
        //执行延时任务
        int64_t min_delay_ms = getDelayedTaskMinDelay();
        
        startSleep();
        semaphore_.waitTime(min_delay_ms);
        sleepWakeUp();
    }
    infof("thread:%s exit!\n", name_.c_str());
}

}