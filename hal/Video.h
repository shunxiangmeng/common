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
#include "Defines.h"
#include "common/mediaframe/MediaDefine.h"
#include "common/mediaframe/MediaFrame.h"
#include "infra/include/Optional.h"
#include "infra/include/Signal.h"

namespace hal {

typedef struct VideoEncodeParams {
    VideoCodecType codec;  // 编码格式
    int32_t width;
    int32_t height;
    int32_t gop;           // I帧间隔
    int32_t bitrate;       // 码率
    float fps;             // 帧率
    VideoEncodeRCMode rc_mode;
    bool operator==(const VideoEncodeParams& other) {
        return (codec == other.codec && width == other.width && height == other.height
            && gop == other.gop && bitrate == other.bitrate && fps == other.fps && rc_mode == other.rc_mode);
    }
} VideoEncodeParams;

class IVideo {
public:

    typedef infra::TSignal<void, int32_t, int32_t, MediaFrame&> VideoStreamSignal;
    typedef VideoStreamSignal::Proc VideoStreamProc;

    static IVideo* instance();

    // 设置为vsdp模式,暂不考虑同时支持正常模式和vdsp模式
    virtual bool setVsdpMode() { return false; };
    virtual bool initialVdsp(std::vector<VideoEncodeParams> &video_encode_params) { return false; };
    virtual bool sendFrameVdsp(MediaFrame &frame) { return false; };

    virtual bool initial(int32_t channel, std::vector<VideoEncodeParams> &video_encode_params, int32_t fps = 25) = 0;
    virtual bool deInitial(int32_t channel = 0) = 0;

    virtual bool setSampleFormat(int32_t channel, VideoSampleFormat sample_format) { return false; }
    /*
    channel: sensor 通道号，目前只支持一个sensor，只能是0
    stream_type: 0-主码流， 1-子码流，2-3码流
    */
    virtual bool setEncodeParams(int32_t channel, int32_t sub_channel, VideoEncodeParams &encode_params) = 0;
    virtual bool getEncodeParams(int32_t channel, int32_t sub_channel, VideoEncodeParams &encode_params) = 0;
    // 强制 I 帧
    virtual bool requestIFrame(int32_t channel, int32_t sub_channel) = 0;

    virtual bool setResolution(int32_t channel, int32_t sub_channel, int32_t width, int32_t height) { return false; }

    virtual bool startStream(int32_t channel, int32_t sub_channel, VideoStreamProc proc) = 0;
    virtual bool stopStream(int32_t channel, int32_t sub_channel, VideoStreamProc proc) = 0;
    virtual bool streamIsStarted(int32_t channel, int32_t sub_channel) = 0;

    // for alg
    typedef struct {
        int32_t buffer_size;
        int32_t width;
        int32_t height;
        int32_t stride;
        int64_t timestamp;
        uint32_t frame_number;
        int32_t data_size;
        uint8_t *data;
    } VideoImage;

    //virtual bool waitViImage(int32_t channel, int32_t sub_channel, int32_t timeout = -1) = 0;
    virtual bool getViImage(int32_t channel, int32_t sub_channel, VideoImage &image, int32_t timeout = -1) = 0;
};

}