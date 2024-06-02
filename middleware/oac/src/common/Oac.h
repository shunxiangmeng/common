/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Oac.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 17:16:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include "Semaphore.h"
#include "infra/include/Logger.h"
#include "hal/Defines.h"

namespace oac {

#define PICTURE_MAX     32

typedef struct {
    bool empty;
    bool busy;
    int32_t index;
    IMAGE_PIXEL_FORMAT format;
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t size;
    int64_t timestamp;
    uint32_t frame_number;
    int32_t buffer_size;
} SharedMemoryPictureHead;


typedef struct {
    uint8_t tag[4];        // @@@@
    int32_t total_size;    // 共享内存总长
    int32_t picture_sum;
    int32_t picture_count; // 队列中的图片数量
    int32_t write_index;   // 写索引
    int32_t read_index;    // 读索引
} SharedMemoryHead;


typedef struct SharedImage {
    Semaphore sem;
    std::string sem_name;
    SharedMemoryPictureHead* shared_picture;
    uint8_t* data_addr;
    SharedImage(const std::string& name): sem(name), shared_picture(nullptr), data_addr(nullptr) {
    }
    ~SharedImage() {
        tracef("~SharedImage()\n");
    }
    void wait() {
        sem.wait();
    }
    void release() {
        sem.release();
    }
} SharedImage;




}