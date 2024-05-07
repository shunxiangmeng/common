/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Signal.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-23 23:18:51
 * Description :  一个信号被触发时，会调用所有连接到其上的 function 对象
 * Others      :  
 ************************************************************************/
#pragma once
#include <stdio.h>
#include <functional>
#include <mutex>
#include <vector>
#include "Logger.h"
#include "Timestamp.h"
namespace infra {

template <typename R, typename... Args>
class TSignal {
    /* 信号节点状态 */
    enum SlotState {
        slotStateEmpty,     /* 节点为空   */
        slotStateNormal     /* 节点已连接 */
    };

public:
    /* 节点挂载的位置 */
    enum SlotPosition {
        any,                /* 任意地方   */
        back,               /* 挂载在末尾 */
        front               /* 挂载在头部 */
    };

    /* 信号操作错误码 */
    enum ErrorCode {
        errorNoFound = -1,      /* 没有找到制定的对象 */
        errorExist = -2,        /* 对象已经存在 */
        errorFull = -3,         /* 已经到达能够连接的对象个数上限 */
        errorEmptyProc = -4,    /* 对象包含的函数指针为空，没有意义 */
        errorAllReuseProc = -5  /* 任意远程函数指针，没有意义 */
    };

    /* 与信号模板参数类型匹配的函数指针对象类型 */
    typedef std::function<R(Args...)> Proc;

    typedef struct TProc {
        Proc proc;
        void *function_pointer;
    public:
        TProc() {}
        template<typename X>
        TProc(R(X::*method)(Args...), X* pthis) {
            proc = [](const auto& method, auto* pthis) {
                return [=](auto&&... args) -> decltype(auto) {
                    return (obj->*mem_fun_ptr)(std::forward<decltype(args)>(args)...);
                };
            };
            function_pointer = (void*)&method;
        }
        ~TProc() {}
    } TProc;


    /*auto Func0 = [](const auto& mem_fun_ptr, auto* obj) {
        void* pointer = (void*)&mem_fun_ptr;
        auto p = [=](auto&&... args) -> decltype(auto) {
            return (obj->*mem_fun_ptr)(std::forward<decltype(args)>(args)...);
        };
        return p;
    };*/

private:
    /* 信号节点结构 */
    struct SignalSlot {
        Proc proc;
        SlotState state;
        union {
            uint8_t count;	     /* 为0时表示回调函数未被调用或回调函数已经返回 */
            uint32_t placeholder;
        };
        uint32_t cost;
    };

    struct SignalSlotV2 {
        TProc proc;
    };
    std::vector<SignalSlotV2> slotsV2_;


    int number_max_;
    int number_;
    std::vector<SignalSlot> slots_;
    std::mutex mutex_;
    int64_t	thread_id_ = std::hash<std::thread::id>()(std::this_thread::get_id());

public:
    /* @param max_slots 能够连接的最大函数指针对象的个数 */
    TSignal(int max_slots = 8) : number_max_(max_slots), number_(0), thread_id_(-1) {
        slots_.reserve(max_slots);
        slots_.resize(max_slots);
        for (int i = 0; i < number_max_; i++) {
            slots_[i].state = slotStateEmpty;
            slots_[i].placeholder = 0;
            slots_[i].count = 0;
        }

        slotsV2_.reserve(4);
        slotsV2_.resize(4);
    }

    ~TSignal() {
        //delete []slots_;
    }

    /* 挂载函数指针对象
     * @param proc 函数指针对象
     * @param position 对象挂载位置
     * @retval >=0 调用后已经挂载到信号的函数指针对象个数
     * @retval <0 errorCode类型的错误码 */
    int attach(const Proc &proc, SlotPosition position = any) {
        if (!proc) {
            return errorEmptyProc;
        }
        if (isAttached(proc)) {
            return errorExist;
        }

        std::lock_guard<std::mutex> guard(mutex_);
        switch (position) {
        case any:
            for(int i = 0; i < number_max_; i++) {
                if (slots_[i].state == slotStateEmpty && slots_[i].count == 0) {
                    slots_[i].proc  = proc;
                    slots_[i].state = slotStateNormal;
                    return ++number_;
                }
            }
            break;
        case back:
            for (int i = number_max_ - 1; i >= 0; i--) {
                if (slots_[i].state == slotStateEmpty && slots_[i].count == 0) {
                    for (int j = i; j < number_max_ - 1; j++) {
                        slots_[j] = slots_[j + 1];
                    }
                    slots_[number_max_ - 1].proc  = proc;
                    slots_[number_max_ - 1].state = slotStateNormal;
                    return ++number_;
                }
            }
            break;
        case front:
            for (int i = 0; i < number_max_; i++) {
                if (slots_[i].state == slotStateEmpty && slots_[i].count == 0) {
                    for (int j = i; j > 0; j--) {
                        slots_[j] = slots_[j - 1];
                    }
                    slots_[0].proc  = proc;
                    slots_[0].state = slotStateNormal;
                    return ++number_;
                }
            }
            break;
        }
        return errorFull;
    }

    int attachV2(const TProc& proc) {

        return 0;
    }

    /* 卸载函数指针对象，根据对象中保存的函数指针来匹配
     * @param proc 函数指针对象
     * @param wait 是否等待正在进行的回调结束。一般在使用者对象析构的时候需要等待
     *             如果是在回调函数里卸载，则不能等待。等待要特别小心，防止死锁
     * @retval >=0 调用后已经挂载到信号的函数指针对象个数
     * @retval <0 errorCode类型的错误码 */
    int detach(const Proc &proc, bool wait = false) {
        if (!proc) {
            return errorEmptyProc;
        }

        bool anyRemoteRemoved = false;
        std::lock_guard<std::mutex> guard(mutex_);
        for (int i = 0; i < number_max_; i++) {
            auto proc0 = slots_[i].proc.target<R(*)(Args...)>();
            //proc0 = slots_[i].proc.target<Proc)>();
            std::string s = typeid(decltype(proc)).name();
            auto proc1 = proc.target<R(*)(Args...)>();
            //auto proc0 = slots_[i].proc;
            //auto proc1 = proc;
            if (proc0 && proc0 == proc1 && slots_[i].state == slotStateNormal) {
                /* 回调线程和stop线程不是同一线程时，才需要等待，否则等待会引起自锁 */
                if (wait && slots_[i].count && std::hash<std::thread::id>()(std::this_thread::get_id()) != thread_id_) {
                    while (slots_[i].count && slots_[i].state == slotStateNormal) {
                        mutex_.unlock();
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        infof("Signal::Detach wait callback exit!\n");
                        mutex_.lock();
                    }
                }

                slots_[i].state = slotStateEmpty;
                return --number_;
            }
        };
        if (anyRemoteRemoved) { /* 存在复用函数 */
            return number_;
        }
        return errorNoFound;
    }

    /* 判断卸载函数指针对象是否挂载，根据对象中保存的函数指针来匹配
     * @param proc 函数指针对象 */
    bool isAttached(const Proc &proc) {
        if (!proc) {
            return false;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        for (int i = 0; i < number_max_; i++) {
            auto proc0 = slots_[i].proc.target<R(*)(Args...)>();
            std::string type_name = proc.target_type().name();
            std::string s = typeid(decltype(proc)).name();
            infof("type_name:%s\n", type_name.data());
            infof("s:%s\n", s.data());
            auto proc1 = proc.target<R(*)(Args...)>();
            if (proc0 && proc0 == proc1 && slots_[i].state == slotStateNormal) {
                return true;
            }
        }
        return false;
    }

    /* 重载()运算符，可以以函数对象的形式来调用连接到信号的所有函数指针 */
    inline void operator()(Args... args) {
        std::lock_guard<std::mutex> guard(mutex_);
        thread_id_ = std::hash<std::thread::id>()(std::this_thread::get_id());
        // call back functions one by one
        for (int i = 0; i < number_max_; i++) {
            if (slots_[i].state == slotStateNormal) {
                Proc temp = slots_[i].proc;
                slots_[i].count++;
                mutex_.unlock();
                /* 函数执行与性能统计 (cost在不进行时间统计的情况下为0) */
            #ifdef SIGNAL_COST
                uint64_t us1 = infra::getCurrentTimeUs();
                temp(args...);
                uint64_t us2 = infra::getCurrentTimeUs();
                slots_[i].cost = (us1 <= us2) ? uint32_t(us2 - us1) : 1;
            #else
                temp(args...);
                slots_[i].cost = 0;
            #endif
                mutex_.lock();
                slots_[i].count--;
                //assert(slots_[i].count >= 0);
            }
        }
    }

    void stat() {
        std::lock_guard<std::mutex> guard(mutex_);
        for (int i = 0; i < number_max_; i++) {
            if (slots_[i].state == slotStateNormal) {
                infof("\t%8d us\n", slots_[i].cost);
            }
        }
    }
};

} // namespace infra

auto Func = [] (const auto& mem_fun_ptr, auto* obj) {
    return [=] (auto&&... args) -> decltype(auto) {
        return (obj->*mem_fun_ptr)(std::forward<decltype(args)>(args)...);
    };
};

auto FuncV2 = [] (const auto& mem_fn_ptr, auto& obj) {
    return [=] (auto&&... args) -> decltype(auto) {
        return (obj.*mem_fn_ptr)(std::forward<decltype(args)>(args)...);
    };
};

#define MemberFunc(func, obj) std::bind(func, obj, std::placeholders::_1)
#define MemberFunc2(func, obj) std::bind(func, obj, std::placeholders::_1, std::placeholders::_2)
#define MemberFunc3(func, obj) std::bind(func, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)