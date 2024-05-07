/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSubSession.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:31:21
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <string>
#include "PrivSessionBase.h"
#include "stream/mediasession/MediaSession.h"
#include "infra/include/Signal.h"
#include "ulucuframe/UlucuPack.h"

class PrivSubSession {
    friend class PrivSession;
public:
    /**
     * @brief 构造
     * @param master
     * @param name
     * @param sessionId
     */
    PrivSubSession(PrivSessionBase *master, const char* name, uint32_t sessionId);
    /**
     * @brief 析构
     */
    ~PrivSubSession();

    typedef infra::TSignal<void, std::string> CallbackSignal;

private:
    /**
     * @brief 开始流
     * @param channel
     * @param streamType
     * @return
     */
    bool startStream(int32_t channel, int32_t streamType);
    /**
     * @brief 停止流
     * @param channel
     * @param streamType
     * @return
     */
    bool stopStream(int32_t channel, int32_t streamType);
    /**
     * @brief 停止流
     * @return
     */
    bool stopStream();
    /**
     * @brief
     * @return
     */
    bool streaming();
    /**
     * @brief 销毁
     */
    void destory();
    /**
     * @brief 获取sessionid
     * @return
     */
    uint32_t getSessionId() const { return mSessionId; }
    /**
     * @brief 获取sessionid
     * @return
     */
    bool setCallback(CallbackSignal::Proc);
private:
    /**
     * @brief 数据回调
     * @param index
     * @param frame
     */
    void onData(MediaFrameType type, MediaFrame &frame);

private:

    bool                            mStreaming;
    uint32_t                        mSessionId;
    PrivSessionBase*               mSessionBase;
    std::shared_ptr<MediaSession> media_session_;
    uint32_t                        mSendFrameCount;
    CallbackSignal::Proc            mCallback;

    std::shared_ptr<UlucuPack>  ulucu_packager_;
};

typedef std::shared_ptr<PrivSubSession>    PrivSubSessionPtr;
