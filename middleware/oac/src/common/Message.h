/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacMessage.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-01 20:01:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <vector>
#include "Oac.h"
#include "msgpack/msgpack.hpp"

struct SharedImageInfo {
    std::string shared_memory_path;
    std::string shared_image_sem_prefix;
    int32_t width;
    int32_t height;
    IMAGE_PIXEL_FORMAT format;
    int32_t count;
    MSGPACK_DEFINE(shared_memory_path, shared_image_sem_prefix, width, height, format, count);
};

struct SharedMemoryInfo {
    std::string path;
    std::vector<std::string> image_sems;
    MSGPACK_DEFINE(path, image_sems);
};

