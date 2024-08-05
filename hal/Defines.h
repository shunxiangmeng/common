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
#include <vector>

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

typedef enum {
    E_TargetShapType_rect,
    E_TargetShapType_rect_pose,
} TargetShapType;

typedef struct {
    float x;      //0 -> 1.0f
    float y;      //0 -> 1.0f
    float w;
    float h;
} Rect;

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    TargetShapType shap_type;
    TargetType type;
    uint32_t id;
    Rect rect;
    std::vector<Point> points;
} Target;

typedef struct {
    int64_t timestamp;
    std::vector<Target> targets;
} CurrentDetectResult;

typedef struct {
    uint32_t color;  //rgb888
    Rect rect;
} DetectRegion;