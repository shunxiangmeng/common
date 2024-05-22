/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  SharedMemory.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 15:38:51
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace infra {
class SharedMemory {
public:
    explicit SharedMemory(const std::string& path, int64_t size, bool create = false) noexcept;

    ~SharedMemory() noexcept;

    inline bool open() noexcept {
        if (valid()) {
            return true;
        }
        return createOrOpen();
    }

    bool valid() const noexcept;

    void close() noexcept;

    inline int64_t size() const noexcept {
        return size_;
    }

    inline std::string path() const noexcept {
        return path_;
    }

    inline uint8_t* data() noexcept {
        return data_;
    }

private:
    bool createOrOpen() noexcept;
private:
    bool create_ = false;
    std::string path_;
    int64_t size_ = 0;
    uint8_t* data_ = nullptr;
#ifdef _WIN32
    HANDLE handle_ = NULL;
#else
    int fd_ = -1;
#endif
};

}