/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 20:03:38
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "jsoncpp/include/value.h"

class PrivServer {
    /**
     * @brief 构造
     */
    PrivServer();
    /**
     * @brief 析构
     */
    ~PrivServer();
public:
    /**
     * @brief 单例
     */
    static PrivServer* instance();
public:
    /**
     * @brief 启动私有服务
     * @param[in] port 服务监听端口号
     */
    bool start(int32_t port = 7000);
    /**
     * @brief 停止私有服务
     */
    bool stop();
    /**
     * @brief 重启私有服务
     * @param[in] port 服务监听端口号
     */
    bool restart(int32_t port = 7000);
    /**
     * @brief 增加可订阅事件
     * @param[in] name  事件名
     */
    bool addSubscribeEvent(const char* name);
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event  json格式的事件
     */
    bool sendEvent(const char* name, const Json::Value &event);
private:
    void* internal_;
};
