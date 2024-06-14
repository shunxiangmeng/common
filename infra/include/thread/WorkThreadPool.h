/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  WorkThreadPool.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 22:32:35
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "WorkThread.h"
#include "TaskExecutor.h"
#include <map>

namespace infra {

class WorkThreadPool : public TaskExecutor {
public:
    static WorkThreadPool* instance();

    bool init(int32_t thread_num = 0, WorkThread::Priority priority = WorkThread::PRIORITY_HIGHEST, bool affinity = true);
    bool stop();

    virtual bool postTask(Task &&task) override;

    bool async(Task &&task);

    virtual int64_t postDelayedTask(Task &&task, int64_t delay_time_ms) override;

    /**
     * 获取所有线程的负载率
     * @return 所有线程的负载率
     */
    std::vector<int> getThreadLoad();

    std::shared_ptr<TaskExecutor> getExecutor();

private:
    WorkThreadPool();
    ~WorkThreadPool();

private:
    //std::map<std::thread::id, Thread> threads_;
    std::mutex threads_mutex_;

    std::vector<std::shared_ptr<WorkThread>> threads_;

    int32_t thread_num_;
    WorkThread::Priority priority_;
    std::string name_;
    bool affinity_;

    int32_t thread_pos_ = 0;
    bool init_ = false;
};

}