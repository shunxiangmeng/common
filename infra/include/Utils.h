/********************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :   Utils.h
 * Author      :   mengshunxiang 
 * Data        :   2024-02-23 23:13:45
 * Description :   None

 ********************************************************************/
#pragma once
#include <string>
#include <atomic>

namespace infra {
    
//禁止拷贝基类
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    noncopyable(const noncopyable &that) = delete;             //拷贝构造函数
    noncopyable(noncopyable &&that) = delete;                  //移动构造函数
    noncopyable &operator=(const noncopyable &that) = delete;  //拷贝赋值运算符
    noncopyable &operator=(noncopyable &&that) = delete;       //移动赋值运算符
};

std::string ExePath();
std::string ExeDir();

void SetThreadName(const char* name);


template <class C>
class ObjectStatistic {
public:
    ObjectStatistic() {
        ++getCounter();
    }
    ~ObjectStatistic() {
        --getCounter();
    }
    static size_t count() {
        return getCounter().load();
    }
private:
    static std::atomic<size_t>& getCounter();
};

/* 对象个数统计 */
#define ObjectStatisticImpl(Type)  \
    template<> \
    std::atomic<size_t>& infra::ObjectStatistic<Type>::getCounter() { \
        static std::atomic<size_t> instance(0); \
        return instance; \
    }


int32_t getCurrentThreadId();

uint32_t htonl(uint32_t value);
uint16_t htons(uint16_t value);

uint32_t ntohl(uint32_t value);
uint16_t ntohs(uint16_t value);

}
