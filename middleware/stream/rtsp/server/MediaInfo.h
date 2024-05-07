/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaInfo.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:53:31
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include "mediasession/MediaSession.h"

class MediaInfo {
public:
    /**
     * @brief 获取视频编码信息
     * @param[in] session   流
     * @param[out] payload  负载
     * @param[out] name     编码名
     * @param[out] rate     采样率
     */
    static bool getVideoCodecInfo(std::shared_ptr<MediaSession> &session, int32_t &payload, std::string &name, int32_t &rate);
    /**
     * @brief 获取音频编码信息
     * param[in] session 流
     * param[out] payload
     * param[out] name
     * param[out] rate 采样率
     * param[out] channel 通道数
     * return true-使能音频，false-不使能音频
     */
    static bool getAudioCodecInfo(std::shared_ptr<MediaSession> &session, int32_t &payload, std::string &name, int32_t &rate, int32_t &channel);
    /**
     * @brief 获取音频aac 信息
     * @param[in] config
     * @param[in] len
     * @param[in] freq
     * @param[in] channel
     */
    static void getAACAudioInfo(char *config, int32_t len, uint32_t freq, uint32_t channel);
};
