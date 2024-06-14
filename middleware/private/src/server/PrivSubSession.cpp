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
    to_send_meidaframe_list_ = std::make_shared<MediaFrameList>();
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
    //tracef("onData index:%d, size:%4d, pts:%lld\n", type, frame.size(), frame.dts());
    if (mSessionBase == nullptr) {
        return;
    }
    if (mStreaming == false) {
        return;
    }

    ulucu_packager_->putPacket(type, frame);
    MediaFrame outframe;
    if (ulucu_packager_->getPacket(type, outframe)) {
        to_send_meidaframe_list_->push_back(outframe);
        //tracef("to_send_meidaframe_list_.size();%d\n", to_send_meidaframe_list_->size());

        if (!stream_send_taskexcutor_) {
            stream_send_taskexcutor_ = infra::WorkThreadPool::instance()->getExecutor();
        }

        /* 保证数据的顺序发送，发送任务都压入同一个线程 */
        stream_send_taskexcutor_->postTask([this] () {
            do {
                MediaFrame to_send_frame = to_send_meidaframe_list_->front();
                if (to_send_frame.empty()) {
                    return;
                }

                int32_t ret = mSessionBase->sendStream((const char*)to_send_frame.data(), to_send_frame.size(), to_send_frame.reserve());
                if (ret < 0) {
                    errorf("onData send failed ret:%d\n", ret);
                    if (!mCallback.empty()) {
                        mCallback("send error");
                    }
                    mStreaming = false;
                    return;
                } else if (ret > 0) {
                    to_send_meidaframe_list_->pop_front();
                }

                const int32_t list_max_frame = 200;
                // socket is blocked or the network is too show 
                if (to_send_meidaframe_list_->size() > list_max_frame) {
                    warnf("to_send_meidaframe_list_.size:%d > %d\n", to_send_meidaframe_list_->size(), list_max_frame);
                    int32_t discard_frame_count = 0;
                    do {
                        MediaFrame frame = to_send_meidaframe_list_->front();
                        if (discard_frame_count != 0 && frame.isKeyFrame()) {
                            break;
                        }
                        to_send_meidaframe_list_->pop_front();
                        discard_frame_count++;
                    } while (to_send_meidaframe_list_->size() > 0);
                    warnf("discard_frame:%d\n", discard_frame_count);
                }
            } while (to_send_meidaframe_list_->size() > 0);
        });
    }
}

bool PrivSubSession::streaming() {
    return mStreaming;
}
