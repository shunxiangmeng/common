/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivWebsocketSession.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:33:28
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string>
#include <stdio.h>
#include "jsoncpp/include/value.h"
#include "Sha1.h"
#include "infra/include/Base64.h"
#include "PrivWebsocketSession.h"
#include "infra/include/Logger.h"
#include "infra/include/thread/WorkThreadPool.h"

PrivWebsocketSession::PrivWebsocketSession(IPrivSessionManager *manager,uint32_t sessionId) : PrivSession(manager, "PrivWebsocket", sessionId), 
    mSock(nullptr), mSessionId(sessionId){
    mRecvBufferDataLen = 0;
    mRecvBufferLen = 16 * 1024;
    mRecvBuffer = std::shared_ptr<char>(new char[mRecvBufferLen]);
}

PrivWebsocketSession::~PrivWebsocketSession() {
    tracef("~PrivWebsocketSession\n");
}

PrivSession* PrivWebsocketSession::createCPrivSession(){
    return nullptr;
}

bool PrivWebsocketSession::initial(NetSocketPtr& newSock, const char* buffer, int32_t len){
    tracef("PrivWebsocketSession init+++\n");

    mSock = newSock;
    PrivSession::initial(newSock, 64 * 1024, true);

    setProxy(this);

    getUpgradeResponse(buffer, len);

    return true;
}

int32_t PrivWebsocketSession::getUpgradeResponse(const char *request, int32_t len){
    tracef("getUpgradeResponse++\n");
    char buffer[64] = {0};
    char *find = strstr((char*)request, "Sec-WebSocket-Key:");
    if (find && sscanf(find, "Sec-WebSocket-Key: %s", buffer) == 1){
        //tracef("Sec-WebSocket-Key: %s\n", buffer);
    } else {
        errorf("not get Sec-WebSocket-Key\n");
        return -1;
    }
    
    const char *mask = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string websocketkey = buffer;
    websocketkey += mask;

    uint32_t hash[5] = {0};
    CSha1 websocketsha;
    websocketsha.Init();
    websocketsha.Update((const uint8_t*)websocketkey.c_str(), (uint32_t)websocketkey.size());
    websocketsha.Final(hash);

    std::string websocketaccept = infra::base64Encode((uint8_t const*)hash, sizeof(hash));

    int32_t resBufLen = 512;
    std::shared_ptr<char> responseBuffer(new char[resBufLen]);
    char *buf = responseBuffer.get();
    snprintf(buf, resBufLen,  "HTTP/1.1 101 Switching Protocols\r\n"
                                "Upgrade: WebSocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Access-Control-Allow-Origin: *\r\n"
                                "Sec-WebSocket-Accept: %s\r\n\r\n", websocketaccept.c_str());
    tracef("response:\n%s\n", buf);
    sendCommand(buf, (int32_t)strlen(buf));
    return len;
}

///接收数据并解析成私有帧
int32_t PrivWebsocketSession::recvData(NetSocketPtr& sock, char* frameBuffer, uint32_t frameBufferLen){
    char *buf = mRecvBuffer.get();
    int32_t ret = sock->recv(buf + mRecvBufferDataLen, mRecvBufferLen - mRecvBufferDataLen);

    if (ret <= 0){
        return ret;
    }
    tracef("recv data len:%d\n", ret);
    mRecvBufferDataLen += ret;
    int32_t len = mRecvBufferDataLen;
    int32_t used = 0;
    int32_t usedSum = 0;
    char* frameStart = nullptr;
    int32_t frameLen = 0;
    int32_t usedFrameBufferLen = 0;
    do{
        used = parse(buf + usedSum, len, frameStart, frameLen);
        if (used > 0){
            if (frameLen > ((int32_t)frameBufferLen - usedFrameBufferLen)){  //外部传入的buffer不够用了，不再拷贝数据
                warnf("frameLen(%d) > frameBufferLen(%d) - usedFrameBufferLen(%d)\n", frameLen, frameBufferLen, usedFrameBufferLen);
                break;
            }
        }
        if (frameLen <= 0){
            //client close websocket
            return -1;
        }

        memcpy(frameBuffer + usedFrameBufferLen, frameStart, frameLen);
        usedFrameBufferLen += frameLen;
        usedSum += used;
        len -= used;
    }while(used > 0 && len > 0);
    mRecvBufferDataLen -= usedSum;
    if (mRecvBufferDataLen > 0 && usedSum > 0){
        memcpy(buf, buf + usedSum, mRecvBufferDataLen);
    }
    return usedFrameBufferLen;
}

///解析websocket协议
int32_t PrivWebsocketSession::parse(char* buffer, int32_t size, char* &outBuf, int32_t &outSize) {
    uint8_t fristLen = 0;
    uint64_t dataLen = 0;
    uint32_t pos = 0;
    bool mask = false;
    uint8_t opcode = 0;
    uint8_t maskKey[4];

    opcode = buffer[pos++] & 0x0f;
    if (opcode & 0x08) {
        warnf("client close websocket\n");
        outSize = 0;
        outBuf = nullptr;
        return size;
    }
    if (buffer[pos] & 0x80) {
        mask = true;
    }
    dataLen = fristLen = buffer[pos++] & 0x7f;
    if (fristLen == 126) {
        dataLen = (buffer[pos++] << 8);
        dataLen |= buffer[pos++];
    }
    else if (fristLen == 127) {
        dataLen = ((uint64_t)buffer[pos++] << 56);
        dataLen |= ((uint64_t)buffer[pos++] << 48);
        dataLen |= ((uint64_t)buffer[pos++] << 40);
        dataLen |= ((uint64_t)buffer[pos++] << 32);
        dataLen |= ((uint64_t)buffer[pos++] << 24);
        dataLen |= ((uint64_t)buffer[pos++] << 16);
        dataLen |= ((uint64_t)buffer[pos++] << 8);
        dataLen |= buffer[pos++];
    }
    if (mask){
        maskKey[0] = buffer[pos++];
        maskKey[1] = buffer[pos++];
        maskKey[2] = buffer[pos++];
        maskKey[3] = buffer[pos++];
    }
    if ((uint32_t)size < pos + dataLen){
        tracef("not enough one frame\n");
        return 0;
    }
    unMask(maskKey, &buffer[pos], (int32_t)dataLen);
    outBuf = buffer + pos;
    outSize = (int32_t)dataLen;
    return int32_t(pos + dataLen);
}

int32_t PrivWebsocketSession::process(char* buffer, int32_t size){
    uint8_t fristLen = 0;
    uint64_t dataLen = 0;
    uint32_t pos = 0;
    bool mask = false;
    uint8_t opcode = 0;
    uint8_t maskKey[4];

    opcode = buffer[pos++] & 0x0f;
    if (opcode & 0x08) {
        warnf("client close websocket\n");
    }
    if (buffer[pos] & 0x80) {
        mask = true;
    }
    dataLen = fristLen = buffer[pos++] & 0x7f;
    if (fristLen == 126) {
        dataLen = (buffer[pos++] << 8);
        dataLen |= buffer[pos++];
    }
    else if (fristLen == 127) {
        dataLen = ((uint64_t)buffer[pos++] << 56);
        dataLen |= ((uint64_t)buffer[pos++] << 48);
        dataLen |= ((uint64_t)buffer[pos++] << 40);
        dataLen |= ((uint64_t)buffer[pos++] << 32);
        dataLen |= ((uint64_t)buffer[pos++] << 24);
        dataLen |= ((uint64_t)buffer[pos++] << 16);
        dataLen |= ((uint64_t)buffer[pos++] << 8);
        dataLen |= buffer[pos++];
    }
    if (mask){
        maskKey[0] = buffer[pos++];
        maskKey[1] = buffer[pos++];
        maskKey[2] = buffer[pos++];
        maskKey[3] = buffer[pos++];
    }
    if ((uint32_t)size < pos + dataLen){
        tracef("not enough one frame\n");
        return 0;
    }
    unMask(maskKey, &buffer[pos], (int32_t)dataLen);

    int32_t used = 0;
    std::shared_ptr<Message> msg = PrivSessionBase::parse(&buffer[pos], (int32_t)dataLen, used);
    if (msg.get()){
        if (msg->isMediaData){
            onMediaData(msg, msg->mediaData, msg->mediaDataLen);
        }else{
            if (msg->isResponse){
                onResponse(msg);
            }
            else{
                onRequest(msg);
            }
        }
    }
    return int32_t(pos + dataLen);
}

///内部函数，由调用者保证参数的有效性
void PrivWebsocketSession::unMask(uint8_t maskKey[4], char *buffer, int32_t size){
    for (auto i = 0; i < size; i++){
        buffer[i] = maskKey[i % 4] ^ buffer[i];
    }
}

void PrivWebsocketSession::closeSession(){
    //close();   ///关闭会话，停止会话下的所有发流
}

int32_t PrivWebsocketSession::sendData(const char* buffer, int32_t size){
    uint64_t len = size;
    uint8_t head[32] = {0};
    uint32_t pos = 0;
    head[pos++] = 0x80 + 0x02;
    if (len < 126){
        head[pos++] = len & 0x7F;
    }
    else if (len <= 0xFFFF){
        head[pos++] = 126;
        head[pos++] = uint8_t(len >> 8);
        head[pos++] = uint8_t(len);
    }
    else{
        head[pos++] = 127;
        head[pos++] = uint8_t(len >> 56);
        head[pos++] = uint8_t(len >> 48);
        head[pos++] = uint8_t(len >> 40);
        head[pos++] = uint8_t(len >> 32);
        head[pos++] = uint8_t(len >> 24);
        head[pos++] = uint8_t(len >> 16);
        head[pos++] = uint8_t(len >> 8);
        head[pos++] = uint8_t(len >> 0);
    }
    int32_t ret = sendCommand((char*)head, pos);
    if (ret < 0) {
        return ret;
    }
    //只返回payload长度
    int32_t leftLen = size;
    int32_t count = 0;
    int32_t tryCount = 0;
    do {
        ret = sendCommand(buffer + count, leftLen);
        if (ret < 0) {
            errorf("websocket send ret %d\n", ret);
            return ret;
        } else if (ret == 0) {
            warnf("websocket send ret:%d count:%d leftLen:%d\n", ret, count, leftLen);
            tryCount++;
            if (tryCount >= 3) {
                errorf("websocket send failed, tryCount:%d\n", tryCount);
                return -1;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
            tryCount = 0;
        }
        count += ret;
        leftLen -= ret;
    }while(leftLen > 0);
    return size;
}

void PrivWebsocketSession::onDisconnect() {
    tracef("onDisconnect+++\n");
    infra::WorkThreadPool::instance()->async([&] () {
        PrivSession::closeSession();
        if (mSessionManager) {
            mSessionManager->remove(this->getSessionId());
        }
    });
}
