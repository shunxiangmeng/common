/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Define.h
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 13:38:58
 * Description :  Media相关定义
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

typedef enum {
    InvalidFrameType = -1,
    Video = 0,         // video
    Audio,             // audio
    Image              // image
} MediaFrameType;

typedef enum {
    VideoCBR = 0,      // 定码率
    VideoVBR,          // 变码率
    VideoAVBR
} VideoEncodeRCMode;

typedef enum {
    Annexb = 0,
    AvccHvcc,
} PlacementType;

typedef enum {
    VideoCodecInvalid = 0,
    H264              = 1,
    H265              = 2,
    Mpeg4             = 3,
    MJpeg		      = 4,
    Jpeg		      = 5,
    videoCodecButt
} VideoCodecType;

typedef enum {
    VideoFrame_P = 0,
    VideoFrame_I,
    VideoFrame_B
} VideoEncodeType;

typedef struct {
    VideoCodecType codec;
    VideoEncodeType type;
    int32_t width;
    int32_t height;
} VideoFrameInfo;

/**
 * @brief 图像编码定义
 */
typedef enum {
    ImageCodecInvalid = 0,
    ImageMJPEG        = 1,
    ImageJPED         = 2,
    ImageCodecButt
} ImageCodecType;

/**
* @brief audio encode type
*/
typedef enum {
    AudioCodecInvalid = 0,
    AAC      = 1,
    G711a    = 2,
    G711u    = 3,
    G726     = 4,
    PCM      = 5,
    audioCodecButt
} AudioCodecType;

typedef struct {
    AudioCodecType codec;
    int32_t sample_rate;
    int32_t bit_per_sample;
    int32_t channel_count;
    int32_t channel;
} AudioFrameInfo;
