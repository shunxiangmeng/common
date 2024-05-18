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
#include "MediaSource.h"

MediaSession::MediaSession(int32_t channel, int32_t sub_channel, StreamType type) :
    channel_(channel), sub_channel_(sub_channel), type_(type) {
}

MediaSession::~MediaSession() {
    tracef("~MediaSession\n");
    //MediaSource::instance()->stop(channel_, sub_channel_, onframe, type_);
}

bool MediaSession::create() {
    return true;
}

bool MediaSession::start(OnFrameProc onframe) {
    return MediaSource::instance()->start(channel_, sub_channel_, onframe, type_);
}

bool MediaSession::stop(OnFrameProc onframe) {
    return MediaSource::instance()->stop(channel_, sub_channel_, onframe, type_);
}

bool MediaSession::getVideoEncoderParams(hal::VideoEncodeParams& params) {
    return hal::IVideo::instance()->getEncodeParams(channel_, sub_channel_, params);
}

bool MediaSession::getAudioEncoderParams(hal::AudioEncodeParams& params) {
    return hal::IAudio::instance()->getEncodeParams(params);
    return false;
}
