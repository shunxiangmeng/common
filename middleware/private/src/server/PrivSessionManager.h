/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSessionManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:48:58
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <list>
#include <memory>
#include <string.h>
#include "PrivSession.h"
#include "infra/include/network/AcceptSocket.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"
#include "private/include/RpcServer.h"

class PrivSessionManager :  public IPrivSessionManager, public infra::SocketHandler {

    struct ConnectInfo {
        enum {
            MaxPrevBufferSize = 2048,    ///> 初始最大的缓冲大小
        };
        std::shared_ptr<infra::TcpSocket>  sock;                   ///> 套接字对象
        char                          request[MaxPrevBufferSize];  ///> 初始的缓冲
        int                           request_len;                  ///> 初始的缓冲请求大小
        ConnectInfo() {
            memset(request, 0, sizeof(request));
            request_len = 0;
        }
        ConnectInfo& operator= (const ConnectInfo& a) = delete;
    };

public:
    /**
     * @brief 构造
     */
    PrivSessionManager(RPCServer *rpc_server);
    /**
     * @brief 析构
     */
    virtual ~PrivSessionManager();

    /**
     * @brief 增加新连接
     * param newSock[in] 新连接的sock
     */
    void addNewConnect(std::shared_ptr<infra::TcpSocket> &newsock);
    /**
     * @brief 回收会话
     * @param session
     * @return
     */
    int32_t remove(uint32_t session_id);
    /**
     * @brief 通过sessionId查找session
     * @return PrivSession
     */
    std::shared_ptr<PrivSession> getSessionBySessionId(uint32_t sessionId);
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event  json格式的事件
     */
    bool sendEvent(const char* name, const Json::Value &event);
private:
    /**
     * @brief scoket数据输入回调
     * @param fd
     * @return
     */
    virtual int32_t onRead(int32_t fd);
    /**
     * @brief socket异常回调
     * @param fd
     * @return
     */
    virtual int32_t onException(int32_t fd);

    friend class ProxyServer;
    /**
     * @brief 添加session
     * @param name
     * @param sessionId
     * @return
     */
    std::shared_ptr<PrivSession> addNewSession(const char* name, uint32_t sessionId = 0);

private:
    typedef std::list<PrivSession*>                    PrivSessionList;
    typedef std::shared_ptr<ConnectInfo>                ConnectInfoPtr;
    typedef std::map<uint32_t, std::shared_ptr<PrivSession>> PrivSessionMap;
    typedef std::map<int, ConnectInfoPtr>               PrivSocketMap;

    int32_t                             max_connect_;                 ///最大连接数
    int32_t                             session_count_;
    int64_t                             mCloseSessionTimer;

    PrivSessionMap                      mMainSessionMap;             ///异步销毁
    std::mutex                       mMainSessionMapMutex;
    PrivSessionList                     mToCloseSessionList;
    std::mutex                       mToCloseSessionListMutex;
    std::mutex                       mSockQueueMutex;             ///< mSockQueue互斥锁
    PrivSocketMap                       mSockQueue;                  ///< 新建连接队列(未确认请求类型(HTTP或者RTSP)的队列)

    std::map<int32_t, std::shared_ptr<ConnectInfo>> connect_queue_;       ///< 新建连接队列(未确认请求类型(HTTP或者RTSP)的队列)
    std::mutex connect_queue_mutex_;

    std::map<uint32_t, std::shared_ptr<PrivSession>> session_map_;
    std::mutex session_map_mutex_;

    RPCServer *const rpc_server_;

};
