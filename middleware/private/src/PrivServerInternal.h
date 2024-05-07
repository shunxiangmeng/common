/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivServerInternal.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:16:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include "PrivSubSession.h"
#include "PrivSessionManager.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/AcceptSocket.h"

class PrivInternal : public infra::SocketHandler {
public:
    /**
     * @brief 构造
     */
    PrivInternal();
    /**
     * @brief 析构
     */
    virtual ~PrivInternal();
    /**
     * @brief 启动私有服务
     * @param[in] port 服务监听端口号
     */
    bool start(uint16_t port);
    /**
     * @brief 停止私有服务
     */
    bool stop();
    /**
     * @brief 重启私有服务
     * @param port[in] 服务监听端口号
     */
    bool restart(uint16_t port);
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
    /**
     * @brief 数据输入
     * @param fd
     * @return
     */
    virtual int32_t onRead(int32_t fd);
    /**
     * @brief socket 异常
     * @param fd
     * @return
     */
    virtual int32_t OnException(int32_t fd);

private:
    uint16_t              port_;
    infra::AcceptSocket   acceptor_;
    std::shared_ptr<PrivSessionManager> session_manager_;
};
