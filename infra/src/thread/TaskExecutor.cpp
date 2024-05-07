/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TaskExecutor.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-16 14:32:04
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/thread/TaskExecutor.h"
#include "infra/include/Timestamp.h"

static const int64_t s_max_wait_time = 10 * 1000;  //ms
namespace infra {

int64_t TaskExecutor::postDelayedTask(Task &&task, int64_t delay_time_ms) {
    int64_t task_id = delay_time_ms + getCurrentTimeUs();
    std::lock_guard<decltype(delayed_task_queue_mutex_)> guard(delayed_task_queue_mutex_);
    delayed_task_queue_[task_id] = std::move(task);
    return task_id;
}

bool TaskExecutor::deleteDelayedTask(int64_t task_id) {
    std::lock_guard<decltype(delayed_task_queue_mutex_)> guard(delayed_task_queue_mutex_);
    delayed_task_queue_.erase(task_id);
    return true;
}

int64_t TaskExecutor::getDelayedTaskMinDelay() {
    auto it0 = delayed_task_queue_.begin();
    if (it0 == delayed_task_queue_.end()) {
        return s_max_wait_time;
    }

    int64_t now = getCurrentTimeMs();
    if (it0->first > now) {
        return it0->first - now;
    }

    decltype(delayed_task_queue_) copy;
    delayed_task_queue_mutex_.lock();
    copy.swap(delayed_task_queue_);
    delayed_task_queue_mutex_.unlock();

    //执行延时任务
    for (auto it1 = copy.begin(); it1 != copy.end() && it1->first <= now; it1 = copy.erase(it1)) {
        (it1->second)();
    }

    int64_t delay_time = s_max_wait_time;
    
    delayed_task_queue_mutex_.lock();
    delayed_task_queue_.insert(copy.begin(), copy.end());
    auto it2 = delayed_task_queue_.begin();
    if (it2 != delayed_task_queue_.end()) {
        delay_time = it2->first - getCurrentTimeMs();
    }
    delayed_task_queue_mutex_.unlock();

    return delay_time;
}
}
