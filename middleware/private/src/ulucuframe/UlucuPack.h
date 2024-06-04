/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UlucuPack.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 16:16:13
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "common/mediaframe/MediaFrame.h"

class UlucuPack {
    UlucuPack(const UlucuPack &other);
    UlucuPack& operator= (const UlucuPack &other) = delete;

public:
    UlucuPack();
    ~UlucuPack();

public:
    /**
     * @brief 输入数据块,用于转码
     * @param[in] type
     * @param[in] frame 待转码的数据包
     * @return
     */
    int32_t putPacket(MediaFrameType type, MediaFrame &frame);
    /**
     * @brief 获取转码好的数据
     * @param[in] type
     * @param[in] frame 已转码的数据包
     */
    int32_t getPacket(MediaFrameType type, MediaFrame &frame);

    static MediaFrame getMediaFrameFromBuffer(const char* buffer, int32_t size);

private:
    /**
     * @brief 送入视频
     * @param packInfo
     * @param data
     * @param length
     * @return
     */
    int32_t putVideo(MediaFrame &frame);
    /**
     * @brief 送入音频
     * @param packInfo
     * @param data
     * @param length
     * @return
     */
    int32_t putAudio(MediaFrame &frame);
    /**
     * @brief 采样率
     * @param sample
     * @return
     */
    int8_t audioSample(uint32_t sample);
    /**
     * @brief 视频编码
     * @param codec
     * @return
     */
    int8_t videoCodec(VideoCodecType codec);
    /**
     * @brief 音频编码
     * @param codec
     * @return
     */
    int8_t audioCodec(AudioCodecType codec);

private:
    uint32_t   mSequence[2];
    MediaFrame  mMediaFrame[2];
};
