/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  SessionTimer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-03 20:43:30
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <atomic>
#include <functional>
#include "infra/include/Signal.h"
#include "infra/include/Timer.h"

class SessionTimer {
public:
    SessionTimer();
    virtual ~SessionTimer();
    /**
     * @brief 定时器回调
     */
    typedef std::function<void()> TimerProc;
    /**
     * @brief 启动定时器
     * @param period[in] 定时器超时时间,单位秒
     * @param proc[in]          定时器超时回调
     * @return true-ok, false-failed
     */
    bool start(int32_t period, TimerProc proc);
    /**
     * @brief 停止定时器
     * @return true-ok, false-failed
     */
    bool stop();
    /**
     * @brief 复位定时器计数值
     */
    void reset();
private:
    infra::Timer timer_;
    std::atomic<int32_t>  time_count_;
    int32_t  time_value_;
    TimerProc timer_proc_;
};
