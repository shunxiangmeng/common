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


class IOacClient {
public:
    static IOacClient* instance();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual bool getImageFrame(ImageFrame& image) = 0;
    virtual bool releaseImageFrame(ImageFrame& image) = 0;   //使用完成之后需要调用此接口释image

};

}