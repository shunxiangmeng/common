/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ThreadLoadCounter.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 19:42:31
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/thread/ThreadLoadCounter.h"
#include "infra/include/Timestamp.h"

namespace infra {

ThreadLoadCounter::ThreadLoadCounter(uint64_t max_size, uint64_t max_usec) {
    last_sleep_time_ = last_wake_time_ = getCurrentTimeUs();
    max_size_ = max_size;
    max_usec_ = max_usec;
}

void ThreadLoadCounter::startSleep() {
    std::lock_guard<std::mutex> lock(mutex_);
    sleeping_ = true;
    auto current_time = getCurrentTimeUs();
    auto run_time = current_time - last_wake_time_;
    last_sleep_time_ = current_time;
    time_list_.emplace_back(run_time, false);
    if (time_list_.size() > max_size_) {
        time_list_.pop_front();
    }
}

void ThreadLoadCounter::sleepWakeUp() {
    std::lock_guard<std::mutex> lock(mutex_);
    sleeping_ = false;
    auto current_time = getCurrentTimeUs();
    auto sleep_time = current_time - last_sleep_time_;
    last_wake_time_ = current_time;
    time_list_.emplace_back(sleep_time, true);
    if (time_list_.size() > max_size_) {
        time_list_.pop_front();
    }
}

int ThreadLoadCounter::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t totalSleepTime = 0;
    uint64_t totalRunTime = 0;
    std::for_each(time_list_.begin(), time_list_.end(), [&](const TimeRecord &record) {
        if (record.sleep) {
            totalSleepTime += record.time;
        } else {
            totalRunTime += record.time;
        }
    });

    if (sleeping_) {
        totalSleepTime += (getCurrentTimeUs() - last_sleep_time_);
    } else {
        totalRunTime += (getCurrentTimeUs() - last_wake_time_);
    }

    uint64_t totalTime = totalRunTime + totalSleepTime;
    while ((time_list_.size() != 0) && (totalTime > max_usec_ || time_list_.size() > max_size_)) {
        TimeRecord &record = time_list_.front();
        if (record.sleep) {
            totalSleepTime -= record.time;
        } else {
            totalRunTime -= record.time;
        }
        totalTime -= record.time;
        time_list_.pop_front();
    }
    if (totalTime == 0) {
        return 0;
    }
    return (int)(totalRunTime * 100 / totalTime);
}

}