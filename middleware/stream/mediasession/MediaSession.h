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
#include "media/mediaPlatform.h"

class MediaSession {
public:
    typedef infra::TSignal<void, MediaFrameType, MediaFrame&> MediaSessionSignal;
    typedef MediaSessionSignal::Proc OnFrameProc;

    /**
     * @brief 指定类型通道的媒体流
     * @param channel
     * @param sub_channel
     * @return
     */
    MediaSession(int32_t channel, int32_t sub_channel, StreamType type = LiveStream);
    ~MediaSession() = default;

    bool create();

    bool start(OnFrameProc onframe);

    bool stop(OnFrameProc onframe);

    bool getVideoEncoderParams(media::VideoEncodeParams& params);

    bool getAudioEncoderParams(media::AudioEncodeParams& params);
private:
    void onVideoFrame(MediaFrame &frame);
    void onAudioFrame(MediaFrame &frame);
public:
    int32_t channel_;
    int32_t sub_channel_;
    StreamType type_;

    MediaSessionSignal mediasession_signal_;
};