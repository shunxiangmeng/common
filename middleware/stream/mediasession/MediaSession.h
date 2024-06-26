/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSession.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 12:32:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "infra/include/Signal.h"
#include "common/mediaframe/MediaFrame.h"
#include "MediaSource.h"
#include "hal/Video.h"
#include "hal/Audio.h"

class MediaSession {
public:
    typedef infra::TSignal<void, int32_t, int32_t, MediaFrameType, MediaFrame&> MediaSessionSignal;
    typedef MediaSessionSignal::Proc OnFrameProc;

    /**
     * @brief 指定类型通道的媒体流
     * @param channel
     * @param sub_channel
     * @return
     */
    MediaSession(int32_t channel, int32_t sub_channel, StreamType type = LiveStream);
    ~MediaSession();

    bool create();

    bool start(OnFrameProc onframe);

    bool stop(OnFrameProc onframe);

    bool getVideoEncoderParams(hal::VideoEncodeParams& params);

    bool getAudioEncoderParams(hal::AudioEncodeParams& params);

public:
    int32_t channel_;
    int32_t sub_channel_;
    StreamType type_;

    MediaSessionSignal mediasession_signal_;
};