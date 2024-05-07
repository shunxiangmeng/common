/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Buffer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-24 22:08:43
 * Description :  封装了智能指针的buffer,主要实现了赋值操作
 * Note        :  如果后续需要实现内存管理，则在内存分配释放的地方修改即可
 ************************************************************************/
#pragma once
#include <stdint.h>
#include <memory>
#include <atomic>
#include "Utils.h"

namespace infra {

class Buffer {
public:
    Buffer(int32_t capacity = 0);
    Buffer(const Buffer& other);
    Buffer(Buffer&& other);
    virtual ~Buffer() = default;

    /**
     * @brief 赋值操作
     * @param[in] other 数据源
     * @return 
     */
    void operator=(const Buffer& other);

    bool empty() const {
        return internal_ == nullptr;
    }

    bool ensureCapacity(int32_t capacity);

    int32_t capacity() const;

    int32_t size() const;

    int32_t leftSize() const;

    /**
     * @brief 修改buffer的数据长度，不改变容量，
     * @param[in] size
     * @return 
     */
    void setSize(int32_t size);

    void resize(int32_t size);

    char* data() const;

    char operator[](int32_t index) const;

    int32_t putData(const char *data, int32_t size, bool resize = true);

    int32_t reserve() const;

    void setReserve(int32_t size);

protected:
    struct BufferInternal {
        std::shared_ptr<char[]> buffer;
        int32_t capacity;
        int32_t real_capacity;  //内存对齐之后的大小
        int32_t size;
        int32_t reserve;
        BufferInternal() : capacity(0), real_capacity(0), size(0), reserve(0) {}
    };

    std::shared_ptr<BufferInternal> internal_;
    ObjectStatistic<Buffer> statistic_;
};

class BufferMemoryStatistic {
    friend class Buffer;
    BufferMemoryStatistic() = default;
    ~BufferMemoryStatistic() = default;
public:
    static BufferMemoryStatistic* instance() {
        static BufferMemoryStatistic s_buffer_memory_statistic;
        return &s_buffer_memory_statistic;
    }
    size_t used() const {
        return buffer_used_memory_.load();
    }
private:
    void increase(int32_t count) {
        buffer_used_memory_.fetch_add(count);
    }
    void decrease(int32_t count) {
        buffer_used_memory_.fetch_sub(count);
    }
private:
    std::atomic<size_t> buffer_used_memory_;
};

}

