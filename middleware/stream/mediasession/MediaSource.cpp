/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSource.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 12:39:45
 * Description :  流中心
 * Note        : 
 ************************************************************************/
#include "MediaSource.h"
#include "infra/include/Logger.h"
#include "infra/include/thread/WorkThreadPool.h"

MediaSource* MediaSource::instance() {
    static MediaSource s_mediasource;
    return &s_mediasource;
}

MediaSource::MediaSource() {
}

MediaSource::~MediaSource() {
}

bool MediaSource::start(int32_t channel, int32_t sub_channel, OnFrameProc onframe, StreamType type) {

    if (live_media_signal_.size() < sub_channel + 1) {
        std::lock_guard<std::mutex> guard(mutex_);
        live_media_signal_.push_back(MediaSignal());
    }

    int ret = live_media_signal_[sub_channel].attach(onframe);
    if (ret < 0) {
        errorf("attach media_signal failed ret:%d\n", ret);
        return false;
    }
    if (ret == 1) {
        return hal::IVideo::instance()->startVideoStream(channel, sub_channel, hal::IVideo::MediaStreamProc(&MediaSource::onLiveVideoFrame, this));
    }
    return true;
}

bool MediaSource::stop(int32_t channel, int32_t sub_channel, OnFrameProc onframe, StreamType type) {
    int ret = live_media_signal_[sub_channel].detach(onframe);
    if (ret < 0) {
        errorf("detach media_signal failed ret:%d\n", ret);
        return false;
    }
    if (ret == 0) {
        return hal::IVideo::instance()->stopVideoStream(channel, sub_channel, hal::IVideo::MediaStreamProc(&MediaSource::onLiveVideoFrame, this));
    }
    return true;
}

void MediaSource::onLiveVideoFrame(int32_t channel, int32_t sub_channel, MediaFrame &frame) {
    infra::WorkThreadPool::instance()->async([&, sub_channel, frame] () mutable {
        live_media_signal_[sub_channel](Video, frame);
    });
}

void MediaSource::onLiveAudioFrame(int32_t channel, int32_t sub_channel, MediaFrame &frame) {
    infra::WorkThreadPool::instance()->async([&, sub_channel, frame] () mutable {
        live_media_signal_[sub_channel](Audio, frame);
    });
}
