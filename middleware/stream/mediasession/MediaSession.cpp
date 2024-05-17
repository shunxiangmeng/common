/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSession.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 19:14:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "MediaSession.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"
#include "infra/include/thread/WorkThreadPool.h"

MediaSession::MediaSession(int32_t channel, int32_t sub_channel, StreamType type) :
    channel_(channel), sub_channel_(sub_channel), type_(type) {
    
}

bool MediaSession::create() {

    return true;
}

bool MediaSession::start(OnFrameProc onframe) {
    tracef("mediaSession start+++\n");
    int32_t ret = mediasession_signal_.attach(onframe);
    if (ret <= 0) {
        errorf("start error, attach ret:%d\n", ret);
        return false;
    }
    hal::IVideo::instance()->startVideoStream(channel_, sub_channel_, hal::IVideo::MediaStreamProc(&MediaSession::onVideoFrame, this));
    //media::IMediaPlatform::instance().startAudioStream(media::IMediaPlatform::MediaStreamProc(&MediaSession::onAudioFrame, this));
    //media::IMediaPlatform::instance().requestIFrame(channel_, sub_channel_);
    return true;
}

bool MediaSession::stop(OnFrameProc onframe) {
    //media::IMediaPlatform::instance().stopVideoStream(channel_, sub_channel_, media::IMediaPlatform::MediaStreamProc(&MediaSession::onVideoFrame, this));
    //media::IMediaPlatform::instance().stopAudioStream(media::IMediaPlatform::MediaStreamProc(&MediaSession::onAudioFrame, this));
    int32_t ret = 0;//mediasession_signal_.detach(onframe);
    if (ret < 0) {
        errorf("stop error, detach ret:%d\n", ret);
        return false;
    }
    infof("stop succ, detach ret:%d\n", ret);
    return true;
}

bool MediaSession::getVideoEncoderParams(hal::VideoEncodeParams& params) {
    return hal::IVideo::instance()->getEncodeParams(channel_, sub_channel_, params);
}

bool MediaSession::getAudioEncoderParams(hal::AudioEncodeParams& params) {
    //return media::IMediaPlatform::instance().getAudioEncoderParams(channel_, params);
    return false;
}

void MediaSession::onVideoFrame(MediaFrame &frame) {
    int64_t dts = frame.dts();
    int64_t now = infra::getCurrentTimeMs();
    //warnf("onVideo seq:%d, size:%4d, pts:%lld, now:%lld, diff:%lld\n", frame.sequence(), frame.size(), dts, now, now - dts);
    infra::WorkThreadPool::instance()->async([&, frame] () mutable {
        tracef("signal size:%d\n", mediasession_signal_.size());
        mediasession_signal_(Video, frame);
    });
}

void MediaSession::onAudioFrame(MediaFrame &frame) {
    int64_t dts = frame.dts();
    int64_t now = infra::getCurrentTimeMs();
    warnf("onAudio seq:%d, size:%4d, pts:%lld, now:%lld, diff:%lld\n", frame.sequence(), frame.size(), dts, now, now - dts);
    infra::WorkThreadPool::instance()->async([&, frame] () mutable {
        mediasession_signal_(Audio, frame);
    });
}
