/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspService.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 22:00:46
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "rtsp/include/IRtspService.h"
#include "RtspSessionManager.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/AcceptSocket.h"

class RtspService : public IRtspService, public infra::SocketHandler {
public:
    RtspService();
    virtual ~RtspService();

    /**
     * @brief 启动Rtsp服务
     * @param[in] port 服务监听端口号
     */
    virtual bool start(int32_t port = 554) override;
    /**
     * @brief 停止Rtsp服务
     */
    virtual bool stop() override;
    /**
     * @brief 重启Rtsp服务
     * @param[in] port 服务监听端口号
     */
    virtual bool restart(int32_t port = 554) override;
    /**
     * @brief 设置rtsp是否开启鉴权
     */
    virtual void setAuthority(bool authority) override;
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
