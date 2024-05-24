/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Semaphore.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 19:33:07
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <inttypes.h>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#endif

namespace oac {
struct SemaphoreHandle {
#ifdef _WIN32
    HANDLE h_ = NULL;
#else
    sem_t* named_ = SEM_FAILED;  // Named
    sem_t unnamed_;              // Unnamed
#endif
};

class Semaphore {
public:
    Semaphore(const std::string& name);
    ~Semaphore();

    bool open(const std::string& name, int value = 0) noexcept;
    void close() noexcept;

    bool valid() const noexcept;

    void wait() noexcept;                   // semaphore - 1
    bool wait(const int64_t& ms) noexcept;  // semaphore - 1 , timeout ms
    void release() noexcept;                // semaphore + 1

private:
    bool named_ = false;
    std::string name_;
    SemaphoreHandle* sh_ = nullptr;
};
}
