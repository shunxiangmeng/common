/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspServiceImpl.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 22:00:46
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "RtspSessionManager.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/AcceptSocket.h"

class RtspServiceImpl : public infra::SocketHandler {
public:
    RtspServiceImpl();
    virtual ~RtspServiceImpl();

    /**
     * @brief 启动Rtsp服务
     * @param[in] port 服务监听端口号
     */
    bool start(int32_t port = 554);
    /**
     * @brief 停止Rtsp服务
     */
    bool stop();
    /**
     * @brief 重启Rtsp服务
     * @param[in] port 服务监听端口号
     */
    bool restart(int32_t port = 554);
    /**
     * @brief 打印session
     */
    void dumpsessions();

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
    int32_t               port_;
    infra::AcceptSocket   acceptor_;
    std::shared_ptr<RtspSessionManager> session_manager_;
};
