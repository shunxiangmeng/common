/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  WorkThreadPool.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 22:34:10
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/thread/WorkThreadPool.h"
#include "infra/include/Logger.h"

namespace infra {

WorkThreadPool* WorkThreadPool::instance() {
    static WorkThreadPool s_work_thread_pool;
    return &s_work_thread_pool;
}

WorkThreadPool::WorkThreadPool() : thread_num_(0), priority_(WorkThread::Priority::PRIORITY_NORMAL), affinity_(false){
}

WorkThreadPool::~WorkThreadPool() {
}

bool WorkThreadPool::init(int32_t thread_num, WorkThread::Priority priority, bool affinity) {
    if (init_ == true) {
        warnf("WorkThreadPool already init\n");
        return true;
    }
    init_ = true;
    priority_ = priority;
    affinity_= affinity;
    auto cpus = std::thread::hardware_concurrency();
    thread_num_ = thread_num > 0 ? thread_num : cpus;
    infof("workthread thread num:%d\n", thread_num_);
    for (int32_t i = 0; i < thread_num_; ++i) {
        std::shared_ptr<WorkThread> thread = std::make_shared<WorkThread>(priority, "");
        thread->start();
        threads_.emplace_back(std::move(thread));
    }
    return true;
}

bool WorkThreadPool::stop() {
    for (auto &thread : threads_) {
        thread->stop();
    }
    return true;
}

std::vector<int> WorkThreadPool::getThreadLoad() {
    std::vector<int> vec(threads_.size());
    int i = 0;
    for (auto &thread : threads_) {
        vec[i++] = thread->load();
    }
    return vec;
}

bool WorkThreadPool::postTask(Task &&task) {
    return getExecutor()->postTask(std::move(task));
}

bool WorkThreadPool::async(Task &&task) {
    return postTask(std::move(task));
}

int64_t WorkThreadPool::postDelayedTask(Task &&task, int64_t delay_time_ms) {
    return getExecutor()->postDelayedTask(std::move(task), delay_time_ms);
}

std::shared_ptr<TaskExecutor> WorkThreadPool::getExecutor() {
    auto thread_pos = thread_pos_;
    if (thread_pos >= threads_.size()) {
        thread_pos = 0;
    }
    std::shared_ptr<WorkThread> executor_min_load = threads_[thread_pos];
    auto min_load = executor_min_load->load();

    for (size_t i = 0; i < threads_.size(); ++i) {
        auto th = threads_[thread_pos];
        auto load = th->load();

        if (load < min_load) {
            min_load = load;
            executor_min_load = th;
        }
        if (min_load == 0) {
            break;
        }

        ++thread_pos;
        if (thread_pos >= threads_.size()) {
            thread_pos = 0;
        }
    }

#if 0
    std::string loads = "[";
    for (auto &it : threads_) {
        loads += std::to_string(it->load()) + ",";
    }
    loads[loads.length() - 1] = ']';
    infof("thread_pos:%d, loads:%s\n", thread_pos, loads.data());
#endif
    return executor_min_load;
}

}
