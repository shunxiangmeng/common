/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Timer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-24 20:18:11
 * Description :  使用网络线程池实现的定时器
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include "network/NetworkThreadPool.h"

namespace infra {
class Timer {
public:
    Timer(std::string name = "timer");
    ~Timer();
    /**
     * @brief 函数说明
     * @param[in] period 定时器周期，单位ms
     * @param[in] proc 定时器执行函数,此函数返回true定时器继续，返回false定时器结束
     * @param[in/out] data3
     * @return 
     */
    bool start(int32_t period, const std::function<bool()> &proc);
    bool stop();

private:
    void run(int32_t period, const std::function<bool()> &proc);

private:
    std::shared_ptr<NetworkThread> task_executor_;
    int64_t task_id_;
    bool running_;
    std::string name_;
};
}