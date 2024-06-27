/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSource.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 12:34:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <mutex>
#include <vector>
#include "common/mediaframe/MediaFrame.h"
#include "hal/Video.h"
#include "hal/Audio.h"

typedef enum {
    LiveStream,
    RecordStream,
    RemoteStream,
    FileStream,
} StreamType;

class MediaSource {
    MediaSource();
    ~MediaSource();
public:
    static MediaSource* instance();

    typedef infra::TSignal<void, int32_t, int32_t, MediaFrameType, MediaFrame&> MediaSignal;
    typedef MediaSignal::Proc OnFrameProc;

    bool start(int32_t channel, int32_t sub_channel, OnFrameProc onframe, StreamType type = LiveStream);
    bool stop(int32_t channel, int32_t sub_channel, OnFrameProc onframe, StreamType type = LiveStream);

private:
    void onLiveVideoFrame(int32_t channel, int32_t sub_channel, MediaFrame &frame);
    void onLiveAudioFrame(MediaFrame &frame);

private:
    std::vector<MediaSignal> live_media_signal_;
    std::mutex mutex_;

};