/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  api.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-04 12:51:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <vector>
#include "infra/include/Optional.h"
#include "hal/Defines.h"

bool apiInit();


//@reflection@
struct Response0 {
    int code;
    int tag[32];
    std::string message;
    std::string concept;
    double x;
    float xx;
    int8_t xx2;
    uint32_t y;
    infra::optional<int> optional_z;
    Point point;
    infra::optional<Point> optional_m;
    std::vector<int> vector;
    bool enable;
    float ldkjsnfjn;
};
