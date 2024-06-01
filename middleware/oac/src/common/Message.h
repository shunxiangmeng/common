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
#include "msgpack/msgpack.hpp"

struct SharedMemoryInfo {
    std::string path;
    std::vector<std::string> image_sems;

    MSGPACK_DEFINE(path, image_sems);
};