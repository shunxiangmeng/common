/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Socket.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-16 22:50:42
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>

namespace infra {
class Socket {
public:
    typedef enum {
        unknown,
        tcp,
        udp,
        raw,
        sslTcp,
        simulateStream,
        other
    } SocketType;

    Socket();

    Socket(SocketType type, int32_t fd = -1);

    virtual ~Socket();

    void close(bool notused = true);

    /**
     * @brief 获取 socket 句柄
     * @return fd
     */
    int32_t getHandle() const;

    /**
     * @brief 获取 socket 类型
     * @return @SocketType
     */
    SocketType getType() const;

    /**
     * @brief socket 是否阻塞
     * @return true 阻塞，false 非阻塞
     */
    bool isBlock() const;

    /**
     * @brief socket 是否有效
     * @return
     */
    bool valid() const;

    virtual std::string getLocalIp() const { std::string ip; return ip; };
    virtual uint16_t getLocalPort();

    bool setSendBuffer(int32_t length);
    int32_t getSendBuffer();

    bool setReceiveBuffer(int32_t length);
    int32_t getReceiveBuffer();

    bool setReuseable(bool reuse_addr = true, bool reuse_port = false);
    bool setNoblocked(bool noblock);
    bool setCloExec(bool on = true);

    bool isDomainName(const std::string& str);
    bool isIPAddress(const std::string& str);

protected:
    int32_t         fd_;

private:
    SocketType      type_;
    bool            blocked_;
};

}