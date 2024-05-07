/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TaskExecutor.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 20:01:51
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include <mutex>
#include <map>

namespace infra {

using Task = std::function<void()>;

class TaskExecutor {
public:
    TaskExecutor() = default;
    virtual ~TaskExecutor() = default;

    /**
     * 异步执行任务
     * @param task 任务
     * @return 任务是否添加成功
     */
    virtual bool postTask(Task &&task) = 0;

    /**
     * 异步执行延时任务
     * @param task 任务
     * @param delay_time_ms 延时时间,可能由于线程负载重，延时有误差
     * @return 任务添加后的id
     */
    virtual int64_t postDelayedTask(Task &&task, int64_t delay_time_ms);

    /**
     * 删除执行延时任务
     * @param task_id 任务id
     */
    virtual bool deleteDelayedTask(int64_t task_id);

protected:
    int64_t getDelayedTaskMinDelay();

    std::map<int64_t, Task> delayed_task_queue_;
    std::mutex delayed_task_queue_mutex_;
};
}