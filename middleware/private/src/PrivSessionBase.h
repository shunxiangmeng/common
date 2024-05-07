/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSessionBase.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 23:58:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <mutex>
#include "Defs.h"
#include "jsoncpp/include/json.h"
#include "PrivSessionProxy.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"

typedef std::shared_ptr<Message> MessagePtr;
class PrivSessionBase : public infra::SocketHandler {
public:
    /**
     * @brief 构造
     * @param master
     * @param name
     * @param sessionId
     */
    PrivSessionBase(PrivSessionBase *master, const char* name, uint32_t sessionId);
    /**
     * @brief 析构
     */
    virtual ~PrivSessionBase();
    /**
     * @brief 初始化
     * @param newSock
     * @param bufferLen
     * @param bRecv
     * @return
     */
    virtual bool initial(std::shared_ptr<infra::TcpSocket> &newSock, int32_t bufferLen, bool bRecv = true);
    /**
     * @brief 初始化
     * @param newSock
     * @param buffer
     * @param len
     * @return
     */
    virtual bool initial(std::shared_ptr<infra::TcpSocket>& newSock, const char *buffer, int32_t len);
    /**
     * @brief 设置代理
     * @param proxy
     */
    void setProxy(IPrivSessionProxy *proxy);
    /**
     * @brief 获取代理
     * @return
     */
    IPrivSessionProxy* getProxy();
    /**
     * @brief 回复消息
     * @param msg
     * @return
     */
    bool response(MessagePtr &msg);
    /**
     * @brief 发送数据流
     * @param data
     * @param dataLen
     * @param reserve
     * @return
     */
    int32_t sendStream(const char* data, int32_t dataLen, int32_t reserve = 0);
    /**
     * @brief 发送控制
     * @param data
     * @param dataLen
     * @return
     */
    int32_t sendCommand(const char* data, int32_t dataLen);
    /**
     * @brief 发送请求消息
     * @param msg
     * @return 发送的消息长度
     */
    int32_t sendRequest(Json::Value &body);
    /**
     * @brief 接收请求消息
     * @param msg
     */
    virtual void onRequest(MessagePtr &msg) {}
    /**
     * @brief 接收回复消息
     * @param msg
     */
    virtual void onResponse(MessagePtr &msg) {}
    /**
     * @brief 断开连接
     */
    virtual void onDisconnect() {};
    /**
     * @brief 接收数据流
     * @param msg
     * @param buffer
     * @param size
     */
    virtual void onMediaData(MessagePtr &msg, const char *buffer, uint32_t size) {}
    /**
     * @brief 消息解析
     * @param buffer
     * @param len
     * @param usedLen
     * @return
     */
    static MessagePtr parse(const char* buffer, int32_t len, int32_t &usedLen);
    /**
     * @brief 获取socket
     * @return
     */
    NetSocketPtr getSock() const { return mSock; };
    /**
     * @brief 获取session id
     * @return
     */
    uint32_t getSessionId() const { return mSessionId; }

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
    /**
     * @brief 发送数据
     * @param buffer
     * @param len
     * @return
     */
    int32_t send(const char* buffer, int32_t len);
    /**
     * @brief 设置发送缓冲区带大小
     * @param len
     * @return
     */
    int32_t setSendBufferLen(int32_t len);    
    /**
     * @brief session 退出
     */
    void onExitSession();

private:
    /**
     * @brief 消息解析
     * @return
     */
    MessagePtr parse();
    /**
     * @brief 创建命令
     * @param buffer
     * @param bufferLen
     * @return
     */
    uint32_t makeCmd(char* &buffer, uint32_t bufferLen);
    /**
     * @brief 处理请求消息
     * @param request
     */
    void process(MessagePtr &request);

private:
    bool                                mBRecv;                   ///是否接收数据
    char*                               mBuffer;
    int32_t                             mBufferLen;
    int32_t                             mBufferDataLen;
    char                                mCmdSendBuffer[10*1024];   ///避免频繁申请内存，信令发送bufer使用常驻内存
    uint32_t                            mSequence;                 ///发送流时的序号

    std::string                         mName;                    ///会话名
    uint32_t                            mSessionId;               ///会话ID
    PrivSessionBase                    *mMaster;                 ///主会话
    IPrivSessionProxy                   *mProxy;                  ///代理

    std::shared_ptr<infra::TcpSocket>   mSock;
    std::mutex                       mSendMutex;               ///发送锁，防止多线程发送给，导致协议错乱              
};
