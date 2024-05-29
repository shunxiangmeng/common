/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:16:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include "private/include/IPrivServer.h"
#include "PrivSubSession.h"
#include "PrivSessionManager.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/AcceptSocket.h"

class PrivServer : public IPrivServer, public infra::SocketHandler {
public:
    /**
     * @brief 构造
     */
    PrivServer();
    /**
     * @brief 析构
     */
    virtual ~PrivServer();
    /**
     * @brief 启动私有服务
     * @param[in] port 服务监听端口号
     */
    virtual bool start(uint16_t port = 7000) override;
    /**
     * @brief 停止私有服务
     */
    virtual bool stop() override;
    /**
     * @brief 重启私有服务
     * @param port[in] 服务监听端口号
     */
    virtual bool restart(uint16_t port = 7000) override;
    /**
     * @brief 增加可订阅事件
     * @param[in] name  事件名
     */
    virtual bool addSubscribeEvent(const char* name) override;
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event  json格式的事件
     */
    virtual bool sendEvent(const char* name, const Json::Value &event) override;

    virtual RPCServer& rpcServer() override;
private:
    /**
     * @brief 数据输入
     * @param fd
     * @return
     */
    virtual int32_t onRead(int32_t fd) override;
    /**
     * @brief socket 异常
     * @param fd
     * @return
     */
    virtual int32_t onException(int32_t fd) override;

private:
    uint16_t              port_;
    infra::AcceptSocket   acceptor_;
    RPCServer rpc_server_;
    std::shared_ptr<PrivSessionManager> session_manager_;

    
};
