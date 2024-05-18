/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Audio.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-16 00:00:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include "common/mediaframe/MediaDefine.h"
#include "common/mediaframe/MediaFrame.h"
#include "infra/include/Optional.h"
#include "infra/include/Signal.h"

namespace hal {

typedef struct {
    infra::optional<AudioCodecType> codec;  // 编码格式
    infra::optional<int32_t> channel_count; // 通道数量
    infra::optional<int32_t> sample_rate;
    infra::optional<int32_t> bit_per_sample;
} AudioEncodeParams;

class IAudio {
public:

    typedef infra::TSignal<void, MediaFrame&> AudioStreamSignal;
    typedef AudioStreamSignal::Proc AudioStreamProc;

    static IAudio* instance();

    virtual bool initial(AudioEncodeParams &video_encode_params) = 0;
    virtual bool deInitial() = 0;

    /*
    channel: sensor 通道号，目前只支持一个sensor，只能是0
    stream_type: 0-主码流， 1-子码流，2-3码流
    */
    virtual bool setEncodeParams(AudioEncodeParams &encode_params) = 0;
    virtual bool getEncodeParams(AudioEncodeParams &encode_params) = 0;

    virtual bool startStream(AudioStreamProc proc) = 0;
    virtual bool stopStream(AudioStreamProc proc) = 0;

};

}