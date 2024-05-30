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
#include "rpc/RpcServer.h"

class IPrivServer {
public:
    /**
     * @brief 构造
     */
    IPrivServer() = default;
    /**
     * @brief 析构
     */
    virtual ~IPrivServer() = default;
    /**
     * @brief 单例
     */
    static IPrivServer* instance();
    /**
     * @brief 启动私有服务
     * @param[in] port 服务监听端口号
     */
    virtual bool start(uint16_t port = 7000) = 0;
    /**
     * @brief 停止私有服务
     */
    virtual bool stop() = 0;
    /**
     * @brief 重启私有服务
     * @param[in] port 服务监听端口号
     */
    virtual bool restart(uint16_t port = 7000) = 0;
    /**
     * @brief 增加可订阅事件
     * @param[in] name  事件名
     */
    virtual bool addSubscribeEvent(const char* name) = 0;
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event  json格式的事件
     */
    virtual bool sendEvent(const char* name, const Json::Value &event) = 0;

    virtual RPCServer& rpcServer() = 0;
};
