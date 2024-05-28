/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSubSession.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:31:13
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivSubSession.h"
#include "infra/include/Logger.h"

PrivSubSession::PrivSubSession(PrivSessionBase *master,const char* name, uint32_t sessionId) :
                        mStreaming(false), mSessionId(sessionId), mSessionBase(master),
                        mSendFrameCount(0) {
    if (mSessionBase == nullptr) {
        errorf("private base session is nullptr\n");
    }
    ulucu_packager_ = std::make_shared<UlucuPack>();
}

PrivSubSession::~PrivSubSession() {
}

bool PrivSubSession::startStream(int32_t channel, int32_t streamType) {
    if (!media_session_) {
        media_session_ = std::make_shared<MediaSession>(channel, streamType);
    }

    media_session_->create();
    Json::Value info = Json::nullValue;
    //media_session_->config(Muxer::unibStream,info);

    if (!media_session_->start(MediaSession::OnFrameProc(&PrivSubSession::onData, this))) {
        errorf("start streaming error\n");
        return false;        
    }

    mStreaming = true;
    return true;
}

bool PrivSubSession::stopStream(int32_t channel, int32_t streamType){
    if (!media_session_) {
        errorf("media_session_ is nullptr\n");
        return false;
    }
    mStreaming = false;
    return media_session_->stop(MediaSession::OnFrameProc(&PrivSubSession::onData, this));
}

bool PrivSubSession::stopStream() {
    if (!media_session_) {
        errorf("media session is nullptr\n");
        return false;
    }
    mStreaming = false;
    return media_session_->stop(MediaSession::OnFrameProc(&PrivSubSession::onData, this));
}

bool PrivSubSession::setCallback(CallbackSignal::Proc proc) {
    mCallback = proc;
    return true;
}

void PrivSubSession::onData(MediaFrameType type, MediaFrame &frame){
    tracef("onData index:%d, size:%4d, pts:%lld\n", type, frame.size(), frame.dts());
    if (mSessionBase == nullptr) {
        return;
    }
    if (mStreaming == false) {
        return;
    }

    ulucu_packager_->putPacket(type, frame);
    MediaFrame outframe;
    if (ulucu_packager_->getPacket(type, outframe)) {
        int32_t ret = mSessionBase->sendStream((const char*)outframe.data(), outframe.size(), outframe.reserve());
        if (ret < 0) {
            errorf("onData send failed ret:%d\n", ret);
            if (!mCallback.empty()) {
                mCallback("send error");
            }
            mStreaming = false;
        }
    }
}

bool PrivSubSession::streaming() {
    return mStreaming;
}
