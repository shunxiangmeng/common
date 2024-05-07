/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Semaphore.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-24 15:32:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <mutex>
#include <condition_variable>

namespace infra {
class Semaphore {
public:
    Semaphore() {
        count_ = 0;
    }
    ~Semaphore() {}

    void post(int32_t count = 1) {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        count_ += count;
        if (count == 1) {
            condition_.notify_one();
        } else {
            condition_.notify_all();
        }
    }

    void wait() {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        while (count_ == 0) {
            condition_.wait(lock);
        }
        count_--;
    }

    void waitTime(int64_t wait_time_ms) {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        if (count_ == 0) {
            condition_.wait_for(lock, std::chrono::milliseconds(wait_time_ms));
        }
        if (count_ > 0) {
            count_--;
        }
    }

private:
    int32_t count_;
    std::recursive_mutex mutex_;
    std::condition_variable_any condition_;
};

}
