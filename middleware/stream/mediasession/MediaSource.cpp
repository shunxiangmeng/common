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
    {
        std::lock_guard<std::mutex> guard(mutex_);
        int32_t size = live_media_signal_.size();
        if (size < sub_channel + 1) {
            for (int i = 0; i < sub_channel + 1 - size; i++) {
                live_media_signal_.push_back(MediaSignal());
            }
        }
    }

    int ret = live_media_signal_[sub_channel].attach(onframe);
    if (ret == 0) {
        warnf("already attach media_signal\n");
        return true;
    } else if (ret < 0) {
        errorf("attach media_signal failed ret:%d\n", ret);
        return false;
    }
    if (ret == 1) {
        bool v = hal::IVideo::instance()->startStream(channel, sub_channel, hal::IVideo::VideoStreamProc(&MediaSource::onLiveVideoFrame, this));
        bool a = hal::IAudio::instance()->startStream(hal::IAudio::AudioStreamProc(&MediaSource::onLiveAudioFrame, this));
        hal::IVideo::instance()->requestIFrame(channel, sub_channel);
        return v && a;
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
        bool v = hal::IVideo::instance()->stopStream(channel, sub_channel, hal::IVideo::VideoStreamProc(&MediaSource::onLiveVideoFrame, this));
        bool a = hal::IAudio::instance()->stopStream(hal::IAudio::AudioStreamProc(&MediaSource::onLiveAudioFrame, this));
        return v && a;
    }
    return true;
}

void MediaSource::onLiveVideoFrame(int32_t channel, int32_t sub_channel, MediaFrame &frame) {
    infra::WorkThreadPool::instance()->async([&, channel, sub_channel, frame] () mutable {
        live_media_signal_[sub_channel](channel, sub_channel, Video, frame);
    });
}

void MediaSource::onLiveAudioFrame(MediaFrame &frame) {
    infra::WorkThreadPool::instance()->async([&, frame] () mutable {
        for (auto &signal : live_media_signal_) {
            signal(0, 0, Audio, frame);
        }
    });
}
