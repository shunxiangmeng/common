/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ThreadLoadCounter.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 19:38:03
 * Description :  cpu负载计计数器
 * Note        : 
 ************************************************************************/
#pragma once
#include <mutex>
#include <list>

namespace infra {
class ThreadLoadCounter {
public:
    /**
     * 构造函数
     * @param max_size 统计样本数量
     * @param max_usec 统计时间窗口,亦即最近{max_usec}的cpu负载率
     */
    ThreadLoadCounter(uint64_t max_size, uint64_t max_usec);
    ~ThreadLoadCounter() = default;

    /**
     * 线程进入休眠
     */
    void startSleep();

    /**
     * 休眠唤醒,结束休眠
     */
    void sleepWakeUp();

    /**
     * 返回当前线程cpu使用率，范围为 0 ~ 100
     * @return 当前线程cpu使用率
     */
    int load();

private:
    struct TimeRecord {
        TimeRecord(uint64_t time, bool sleep) {
            this->time = time;
            this->sleep = sleep;
        }
        bool sleep;
        uint64_t time;
    };

private:
    bool sleeping_ = true;
    uint64_t last_sleep_time_;
    uint64_t last_wake_time_;
    uint64_t max_size_;
    uint64_t max_usec_;
    std::mutex mutex_;
    std::list<TimeRecord> time_list_;
};
}