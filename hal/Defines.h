/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Defines.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-24 13:33:21
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

typedef enum : int32_t {
    IMAGE_PIXEL_FORMAT_RGB_888 = 0,
    IMAGE_PIXEL_FORMAT_BGR_888,
    IMAGE_PIXEL_FORMAT_ABGR_8888,
    IMAGE_PIXEL_FORMAT_ARGB_8888,
    IMAGE_PIXEL_FORMAT_YUV_420
} IMAGE_PIXEL_FORMAT;

typedef enum {
    E_TargetType_face,
    E_TargetType_body
} TargetType;

typedef struct {
    float x;      //0-1
    float y;      //0-1
    float w;
    float h;
} Rect;

typedef struct {
    TargetType type;
    uint32_t id;
    Rect rect;
} Target;

typedef struct {
    uint32_t timestamp;
    std::vector<Target> targets;
} CurrentDetectResult;
