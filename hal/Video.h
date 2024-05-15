/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Video.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-13 23:59:07
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
    infra::optional<VideoCodecType> codec;  // 编码格式
    infra::optional<int32_t> width;
    infra::optional<int32_t> height;
    infra::optional<int32_t> gop;           // I帧间隔
    infra::optional<int32_t> bitrate;       // 码率
    infra::optional<float> fps;             // 帧率
    infra::optional<VideoEncodeRCMode> rc_mode;
} VideoEncodeParams;

class IVideo {
public:

    typedef infra::TSignal<void, MediaFrame&> MediaStreamSignal;
    typedef MediaStreamSignal::Proc MediaStreamProc;

    static IVideo* instance();

    virtual bool initial(int32_t channel, std::vector<VideoEncodeParams> &video_encode_params) = 0;
    virtual bool deInitial(int32_t channel = 0) = 0;

    /*
    channel: sensor 通道号，目前只支持一个sensor，只能是0
    stream_type: 0-主码流， 1-子码流，2-3码流
    */
    virtual bool setEncodeParams(int32_t channel, int32_t sub_channel, VideoEncodeParams &encode_params) = 0;
    virtual bool getEncodeParams(int32_t channel, int32_t sub_channel, VideoEncodeParams &encode_params) = 0;
    // 强制 I 帧
    virtual bool requestIFrame(int32_t channel, int32_t sub_channel) = 0;

    virtual bool startVideoStream(int32_t channel, int32_t sub_channel, MediaStreamProc proc) = 0;
    virtual bool stopVideoStream(int32_t channel, int32_t sub_channel, MediaStreamProc proc) = 0;

};

}