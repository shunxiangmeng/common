/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Image.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-06 10:42:55
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

namespace hal {
class IImage {
public:
    static IImage* instance();

    virtual bool setInputFramerate(int32_t channel, uint32_t fps) = 0;
    virtual int getInputFramerate(int32_t channel)  = 0;
};
}