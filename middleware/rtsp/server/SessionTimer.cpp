/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  SessionTimer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-03 20:59:26
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "SessionTimer.h"
#include "infra/include/Logger.h"

SessionTimer::SessionTimer() : time_count_(0), time_value_(0) {
}

SessionTimer::~SessionTimer() {
    stop();
}

bool SessionTimer::start(int32_t period, TimerProc proc) {
    timer_proc_ = proc;
    time_count_ = 0;
    time_value_ = period;
    timer_.start(100/*ms*/, [&] () {
        time_count_++;
        if (time_count_ >= time_value_ * 10) {
            if (timer_proc_) {
                timer_proc_();
            }
            time_count_ = 0;
        }
        return true;
    });
    return true;
}

bool SessionTimer::stop() {
    time_count_ = 0;
    timer_.stop();
    return true;
}

void SessionTimer::reset() {
    time_count_ = 0;
}
