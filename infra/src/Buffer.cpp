/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Buffer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-02-24 22:18:01
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string.h>
#include "infra/include/Buffer.h"
#include "infra/include/Logger.h"

namespace infra {

ObjectStatisticImpl(Buffer);

static constexpr int32_t k_algin_size = 4;  //内存对齐

Buffer::Buffer(int32_t capacity) {
    if (capacity) {
        internal_.reset(new BufferInternal(), [](BufferInternal* ptr) {
            BufferMemoryStatistic::instance()->decrease(ptr->real_capacity);
        });
        internal_->capacity = capacity;
        internal_->real_capacity = (capacity + k_algin_size - 1) / k_algin_size * k_algin_size;
        internal_->size = 0;
        internal_->buffer.reset(new char[internal_->real_capacity], std::default_delete<char[]>());

        BufferMemoryStatistic::instance()->increase(internal_->real_capacity);
    }
}

Buffer::Buffer(const Buffer &other) {
    internal_ = other.internal_;
}

Buffer::Buffer(Buffer&& other) {
    internal_ = std::move(other.internal_);
}

void Buffer::operator=(const Buffer& other) {
    internal_ = other.internal_;
}

bool Buffer::ensureCapacity(int32_t capacity) {
    if (internal_ && capacity <= internal_->capacity) {
        return true;
    } else { //扩容或者直接分配
        std::shared_ptr<char[]> buffer_temp;
        int32_t old_data_size = 0;
        int32_t old_real_capacity = 0;
        if (internal_) {
            buffer_temp = internal_->buffer;
            old_data_size = internal_->size;
            old_real_capacity = internal_->real_capacity;
        }

        internal_.reset(new BufferInternal(), [](BufferInternal* ptr) {
            BufferMemoryStatistic::instance()->decrease(ptr->real_capacity);
        });
        internal_->capacity = capacity;
        internal_->real_capacity = (capacity + k_algin_size - 1) / k_algin_size * k_algin_size;
        internal_->size = 0;
        internal_->buffer.reset(new char[internal_->real_capacity], std::default_delete<char[]>());

        if (old_data_size) {
            infof("buffer extention memory from %d to %d\n", old_data_size, capacity);
            memcpy(internal_->buffer.get(), buffer_temp.get(), old_data_size);
            resize(old_data_size);
        }

        BufferMemoryStatistic::instance()->increase(internal_->real_capacity - old_real_capacity);
        return true;
    }
}

int32_t Buffer::capacity() const {
    if (internal_) {
        return internal_->capacity;
    }
    return 0;
}

int32_t Buffer::size() const {
    if (internal_) {
        return internal_->size;
    }
    return 0;
}

int32_t Buffer::leftSize() const {
    if (internal_) {
        return internal_->capacity - internal_->size;
    }
    return 0;
}

int32_t Buffer::reserve() const {
    if (internal_) {
        return internal_->reserve;
    }
    return 0;
}

void Buffer::setReserve(int32_t size) {
    if (internal_ && size <= internal_->capacity) {
        internal_->reserve = size;
    }
}

void Buffer::setSize(int32_t size) {
    if (internal_ && size <= internal_->capacity) {
        internal_->size = size;
    }
}

void Buffer::resize(int32_t size) {
    setSize(size);
}

char* Buffer::data() const {
    if (internal_) {
        return internal_->buffer.get();
    }
    return nullptr;
}

char Buffer::operator[](int32_t index) const {
    if (internal_) {
        if (index > internal_->capacity) {
            errorf("buffer operator index %d > capacity %d\n", index, internal_->capacity);
            return -1;
        }
        return internal_->buffer[index];
    }
    errorf("buffer not init\n");
    return -1;
}

int32_t Buffer::putData(const char *data, int32_t size, bool resize) {
    if (internal_) {
        if (size <= internal_->capacity - internal_->size) {
            memcpy(internal_->buffer.get() + internal_->size, data, size);
            if (resize) {
                internal_->size += size;
            }
            return size;
        }
        errorf("buffer put size %d > remaining size %d\n", size, internal_->capacity - internal_->size);
    }
    return -1;
}

}
