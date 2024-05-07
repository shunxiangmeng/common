/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  SocketHandler.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-19 21:21:32
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "Socket.h"

namespace infra {

class SocketHandler : public std::enable_shared_from_this<SocketHandler> {
public:
    SocketHandler() = default;
    /**
     * @brief 析构函数
     */
    virtual ~SocketHandler();

    /**
     * @brief 事件类型
     */
    typedef enum {
        none        = 0,
        read        = 1,
        write       = 1 << 1,
        except      = 1 << 2,
        timeout     = 1 << 3,
        dealing     = 1 << 4,
        deleted     = 1 << 5
    } EventType;

    virtual int32_t onRead(int32_t fd) { return -1;}

    virtual int32_t onWrite(int32_t fd) { return -1;}

    virtual int32_t onException(int32_t fd) { return -1;}

    virtual int32_t inputTimeout(int32_t fd) { return -1;}

    virtual int32_t outputTimeout(int32_t fd) { return -1;}

    virtual int64_t handleTimeout(int64_t id) { return -1;}

    bool addEvent(Socket& socket, EventType event, int32_t timeout = 0);

    bool delEvent(Socket& socket);

    bool modifyEvent(Socket& socket, EventType event);

private:
    int32_t bind_fd_ = -1;
};

}