/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSessionManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 22:01:23
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <list>
#include <memory>
#include <string.h>
#include <mutex>
#include "RtspSession.h"
#include "infra/include/network/AcceptSocket.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"

class RtspSessionManager : public IRtspSessionManager, public infra::SocketHandler {
public:
    RtspSessionManager();
    virtual ~RtspSessionManager();

    /**
     * @brief 增加新连接
     * @param[in] socket
     */
    void addNewConnect(std::shared_ptr<infra::TcpSocket> &socket);
    /**
     * @brief 回收会话
     * @param session
     * @return
     */
    int32_t remove(std::shared_ptr<RtspSession> session);
    /**
     * @brief 打印所有会话信息
     */
    void dumpSessions();

private:
    /**
     * @brief scoket数据输入回调
     * @param socketFd
     * @return
     */
    virtual int32_t onRead(int32_t fd);
    /**
     * @brief socket异常回调
     * @param socketFd
     * @return
     */
    virtual int32_t onException(int32_t fd);
private:
    std::list<std::shared_ptr<RtspSession>> session_list_;
    std::mutex session_list_mutex_;
    std::list<std::shared_ptr<RtspSession>> to_close_session_list_;
    std::mutex to_close_session_list_mutex_;

    int32_t max_connect_;           ///最大连接数
    int32_t session_count_;         ///已连接会话数量

    struct ConnectInfo {
        enum {
            MaxPrevBufferSize = 2048,    ///> 初始最大的缓冲大小
        };
        std::shared_ptr<infra::TcpSocket>  sock;                        ///> 套接字对象
        char                               request[MaxPrevBufferSize];  ///> 初始的缓冲
        int32_t                            request_len;                 ///> 初始的缓冲请求大小

        ConnectInfo() {
            memset(request, 0, sizeof(request));
            request_len = 0;
        }
        ConnectInfo& operator=(const ConnectInfo& a) = delete;
    };

    std::map<int32_t, std::shared_ptr<ConnectInfo>> connect_queue_;       ///< 新建连接队列(未确认请求类型(HTTP或者RTSP)的队列)
    std::mutex connect_queue_mutex_;
};
