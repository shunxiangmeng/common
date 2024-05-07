/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspService.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:56:08
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

class RtspService {
    RtspService();
    ~RtspService();
public:
    /**
     * @brief 单例
     */
    static RtspService* instance(); 

    /**
     * @brief 启动Rtsp服务
     * param port[in] 服务监听端口号
     */
    bool start(int32_t port = 554);

    /**
     * @brief 停止Rtsp服务
     */
    bool stop();

    /**
     * @brief 重启Rtsp服务
     * param port[in] 服务监听端口号
     */
    bool restart(int32_t port = 554);

private:
    void* internal_;
};
