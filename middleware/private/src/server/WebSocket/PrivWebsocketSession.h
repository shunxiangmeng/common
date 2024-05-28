/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivWebsocketSession.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:33:22
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include "../PrivSession.h"
#include "../PrivSessionProxy.h"
#include "jsoncpp/include/value.h"

class PrivWebsocketSession : public IPrivSessionProxy, public PrivSession {
public:
    PrivWebsocketSession(IPrivSessionManager *manager, uint32_t sessionId);
    PrivWebsocketSession(PrivWebsocketSession &) = delete;
    ~PrivWebsocketSession();

    PrivSession* createCPrivSession();

    bool initial(NetSocketPtr& newSock, const char* buffer, int32_t len);

    virtual int32_t recvData(NetSocketPtr& sock, char* buffer, uint32_t bufferLen);

    virtual int32_t sendData(const char* buffer, int32_t size);

private:
    virtual void onDisconnect();

    int32_t parse(char* buffer, int32_t size, char* &outBuf, int32_t &outSize);

    //virtual int32_t input(int32_t socketFd);   ///由于没有做通用传输通道，就自己收数据

    int32_t getUpgradeResponse(const char *request, int32_t requestLen);

    void unMask(uint8_t maskKey[4], char *buffer, int32_t size);

    int32_t process(char* buffer, int32_t size);

    void closeSession();

private:
    std::shared_ptr<infra::TcpSocket>    mSock;
    std::shared_ptr<char>  mRecvBuffer;        ///接收buffer
    int32_t                mRecvBufferLen;     ///接收buffer长度
    int32_t                mRecvBufferDataLen; ///接收buffer中的数据长度

    //std::shared_ptr<PrivSession> mWorkSession;    ///被代理的会话
    uint32_t mSessionId;

};
