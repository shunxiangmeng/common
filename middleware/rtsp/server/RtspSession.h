/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSession.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 23:11:37
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <memory>
#include "RtspSessionBase.h"
#include "stream/mediasession/MediaSession.h"

class RtspSession;
class IRtspSessionManager {
public:
    IRtspSessionManager() = default;
    virtual ~IRtspSessionManager() {}
    virtual int32_t remove(std::shared_ptr<RtspSession> session) = 0;
};

class RtspSession : public RtspSessionBase, public std::enable_shared_from_this<RtspSession> {
public:
    RtspSession(IRtspSessionManager* manager);
    ~RtspSession();
    bool init(std::shared_ptr<infra::TcpSocket> &newSock, const char *buffer, int32_t len);

private:

    virtual int32_t sendCommand(const char* cmd, int32_t len);

    virtual int32_t startStreaming();

    virtual int32_t stopStreaming();

    virtual int32_t sendRtcp();

    virtual void onMediaData(int32_t chn, int32_t sub_chn, MediaFrameType type, MediaFrame &frame);

    int32_t onMessage(int32_t channel, const char* data, int32_t dataLen);

    void onException(int32_t code);

    virtual void destroySession() override;

    void onMediaDataDestroySession();
private:
    IRtspSessionManager* session_manager_;
    bool send_exception_;
    bool destroied_;
};
