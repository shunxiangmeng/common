/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 19:30:56
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>
#include <vector>
#include "hal/Defines.h"

namespace oac {

//送算法的图像帧
struct ImageFrame {
    int32_t index;
    IMAGE_PIXEL_FORMAT format;
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t size;
    int64_t timestamp;
    uint32_t frame_number;
    uint8_t* data;
};

typedef struct {
    float x;      //0-1
    float y;      //0-1
    float w;
    float h;
} Rect;

typedef enum {
    E_TargetType_face,
    E_TargetType_body
} TargetType;

typedef struct {
    TargetType type;
    uint32_t id;
    Rect rect;
} Target;

typedef struct {
    uint32_t timestamp;
    std::vector<Target> targets;
} CurrentDetectResult;


class IOacClient {
public:
    static IOacClient* instance();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual bool getImageFrame(ImageFrame& image) = 0;
    virtual bool releaseImageFrame(ImageFrame& image) = 0;   //使用完成之后需要调用此接口释image

    virtual bool pushCurrentDetectTarget(CurrentDetectResult& current_result) = 0;  //推送检测结果，用于视频叠加

};

}