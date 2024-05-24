/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OACServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 17:09:51
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>
namespace oac {
class IOacServer {
public:
    static IOacServer* instance();

    /**
     * @brief 函数说明
     * @param[in] sub_channel 使用哪里流送算法
     * @param[in] image_count 共享内存缓存的图片队列长度
     * @param[in/out] data3
     * @return 
     */
    virtual bool start(int32_t sub_channel = 1, int32_t image_count = 3) = 0;
    virtual bool stop() = 0;
};
}
