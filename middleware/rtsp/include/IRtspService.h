/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IRtspService.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:56:08
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

class IRtspService {
public:
    /**
     * @brief 单例
     */
    static IRtspService* instance(); 

    /**
     * @brief 启动Rtsp服务
     * param port[in] 服务监听端口号
     */
    virtual bool start(int32_t port = 554) = 0;

    /**
     * @brief 停止Rtsp服务
     */
    virtual bool stop() = 0;

    /**
     * @brief 重启Rtsp服务
     * param port[in] 服务监听端口号
     */
    virtual bool restart(int32_t port = 554) = 0;

    /**
     * @brief 设置rtsp是否开启鉴权
     */
    virtual void setAuthority(bool authority) = 0;
};
