/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaInfo.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:53:04
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "MediaInfo.h"
#include "infra/include/Logger.h"
#include "stream/mediasession/MediaSession.h"

bool MediaInfo::getVideoCodecInfo(std::shared_ptr<MediaSession> &session, int32_t &payload, std::string &name, int32_t &rate) {
    hal::VideoEncodeParams videoparams;
    session->getVideoEncoderParams(videoparams);
    if (!videoparams.codec.has_value()) {
        errorf("not get video codec type\n");
        payload = 0;
        name = "";
        rate = 0;
        return false;
    }

    if (*videoparams.codec == H264) {
        payload = 96;
        name = "H264";
        rate = 90000;
    } else if (*videoparams.codec == H265) {
        payload = 96;
        name = "H265";
        rate = 90000;
    } else {
        errorf("unknown video codec %d\n", *videoparams.codec);
        payload = 0;
        name = "";
        rate = 0;
        return false;
    }
    return true;
}

bool MediaInfo::getAudioCodecInfo(std::shared_ptr<MediaSession> &session, int32_t &payload, std::string &name, int32_t &rate, int32_t &channel) {
    hal::AudioEncodeParams audioparams;
    session->getAudioEncoderParams(audioparams);
    if (!audioparams.codec.has_value() || !audioparams.sample_rate.has_value() || !audioparams.channel_count.has_value()) {
        errorf("get audio encode info failed\n");
        payload = 0;
        name = "";
        rate = 0;
        return false;
    }
    if (*audioparams.codec == AAC) {
        payload = 97;
        name = "mpeg4-generic";
        rate = *audioparams.sample_rate;
        channel = *audioparams.channel_count;
    } else if (*audioparams.codec == G711a) {
        payload = 8;
        name = "PCMA";
        rate = *audioparams.sample_rate;
        channel = *audioparams.channel_count;
    } else if (*audioparams.codec == G711u) {
        payload = 0;
        name = "PCMU";
        rate = *audioparams.sample_rate;
        channel = *audioparams.channel_count;
    } else {
        errorf("unknown audio codec %d\n", *audioparams.codec);
        payload = 0;
        name = "";
        rate = 0;
        channel = 0;
        return false;
    }
    return true;
}

void MediaInfo::getAACAudioInfo(char *config, int32_t len, uint32_t freq, uint32_t channel) {
    uint8_t tmp[2] = {0};
    /* 采样率类型，在ISO-14496-3 Audio中定义 */
    int32_t freq_type = 0;
    switch (freq)
    {
        case 96000:
            freq_type = 0;
            break;
        case 88200:
            freq_type = 1;
            break;
        case 64000:
            freq_type = 2;
            break;
        case 48000:
            freq_type = 3;
            break;
        case 44100:
            freq_type = 4;
            break;
        case 32000:
            freq_type = 5;
            break;
        case 24000:
            freq_type = 6;
            break;
        case 22050:
            freq_type = 7;
            break;
        case 16000:
            freq_type = 8;
            break;
        case 12000:
            freq_type = 9;
            break;
        case 11025:
            freq_type = 10;
            break;
        case 8000:
            freq_type = 11;
            break;
        case 7350:
            freq_type = 12;
            break;
        default:
            freq_type = 11;
            break;
    }

    /**AAC LC 为2，第1字节的高5为表示AAC类型*/
    tmp[0] = (2 << 3) + (freq_type >> 1);
    tmp[1] = (freq_type << 7) + (channel << 3);
    for (size_t i = 0; i < sizeof(tmp); i++) {
        snprintf(&config[2 * i], len - 2 * i, "%02X", tmp[i]);
    }  
}
