/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ThreadTaskQueue.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 22:19:16
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <mutex>
#include <map>
#include <thread>
#include <string>
#include <queue>
#include "Thread.h"
#include "../Semaphore.h"

namespace infra {

class WorkThread : public Thread, public TaskExecutor, public ThreadLoadCounter {
public:

    WorkThread(Priority priority = PRIORITY_HIGHEST, const std::string &name = "workthread", bool affinity = true);
    virtual ~WorkThread();

    virtual bool postTask(Task &&task) override;
    virtual int64_t postDelayedTask(Task &&task, int64_t delay_time_ms) override;

private:
    virtual void run() override;
    virtual void wakeUp() override;
    std::shared_ptr<Task> getTaskFromeQueue();
    
private:
    std::queue<Task> task_queue_;
    std::mutex task_queue_mutex_;

    Semaphore semaphore_;
};

}