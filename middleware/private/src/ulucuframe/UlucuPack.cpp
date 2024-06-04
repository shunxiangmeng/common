/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UlucuPack.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 16:16:34
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "UlucuPack.h"
#include "Ulucu.h"
#include "UlucuFrame.h"
#include "infra/include/Logger.h"
#include "../Defs.h"

UlucuPack::UlucuPack() {
}

UlucuPack::~UlucuPack() {
}

int32_t UlucuPack::putPacket(MediaFrameType type, MediaFrame &frame) {
    if (type == Video) {
        return putVideo(frame);
    } else if (type == Audio) {
        return putAudio(frame);
    } else {
        return -1;
    }
}

int32_t UlucuPack::getPacket(MediaFrameType type, MediaFrame &frame) {
    if (!mMediaFrame[type].empty()){
        frame = mMediaFrame[type];
    } else {
        return -1;
    }
    mMediaFrame[type].reset();
    return frame.size();
}

int32_t UlucuPack::putVideo(MediaFrame &frame) {
    uint32_t frameLen = sizeof(UlucuFrameHead) + frame.size();
    int trackvideo = 0;
    VideoFrameInfo videoinfo;
    uint8_t naltype = 'P';
    frame.getVideoFrameInfo(videoinfo);
    if (videoinfo.type == VideoFrame_I) {
        frameLen += sizeof(UlucuVideoExHead);
        naltype = 'I';
    }

    uint32_t reserveLen = sizeof(PrivateDataHead);
    frameLen += reserveLen;
    MediaFrame outFrame(frameLen);
    if (outFrame.empty()) {
        errorf("outFrame invalid size:%d\n", frameLen);
        return -1;
    }

    /** 设置一个保留长度，用于填充私有协议头 */
    outFrame.setReserve(reserveLen);
    uint8_t *buffer = (uint8_t*)outFrame.data() + reserveLen;

    UlucuFrame unibFrame(buffer, frameLen - reserveLen);
    unibFrame.setType('V').setSubType(naltype).setSequence(mSequence[trackvideo]);

    float rate = 25.0f;
    int8_t codec = videoCodec(videoinfo.codec);
    if (codec < 0) {
        errorf("not support codec:%d\n", videoinfo.codec);
        return -1;
    }

    VFrameInfo vFrameInfo = {(uint8_t)codec,0,(uint16_t)videoinfo.width,(uint16_t)videoinfo.height, rate, {0,0}};
    unibFrame.setVideoInfo(vFrameInfo);
    unibFrame.setDtsPts(frame.dts(), frame.pts());

    outFrame.setSequence(mSequence[trackvideo]);
    mSequence[trackvideo]++;

    PlacementType placement = frame.placementType();

    uint32_t len = unibFrame.makeFrame((uint8_t*)frame.data(), (uint32_t)frame.size());
    if (len <= 0) {
        errorf("make len <= 0\n");
        return -1;
    }

    outFrame.setMediaFrameType(frame.getMediaFrameType());
    outFrame.setPts(frame.pts());
    outFrame.setDts(frame.dts());
    outFrame.setVideoFrameInfo(videoinfo);
    outFrame.resize(reserveLen + len);
    mMediaFrame[trackvideo] = outFrame;
    return 1;
}

int32_t UlucuPack::putAudio(MediaFrame &frame) {
    int trackAudio = 1;
    AudioFrameInfo info;
    frame.getAudioFrameInfo(info);
    int8_t codec = audioCodec(info.codec);
    if (codec < 0) {
        return -1;
    }

    /** 采样率 */
    int8_t sampleRate = audioSample(info.sample_rate);
    if (sampleRate < 0) {
        return -1;
    }

    uint32_t reserveLen = 0;//ULUCU_PROTOCL_HEAD_LEN;
    uint32_t frameLen = frame.size() + reserveLen;
    frameLen += sizeof(UlucuFrameHead) + sizeof(UlucuAudioExHead);

    MediaFrame outFrame(frameLen);
    if (outFrame.empty()) {
        errorf("outFrame invalid size:%d\n", frameLen);
        return -1;
    }

    /** 设置一个保留长度，用于填充私有协议头 */
    outFrame.setReserve(reserveLen);
    uint8_t *buffer = (uint8_t*)outFrame.data() + reserveLen;

    AFrameInfo frameInfo = {(uint8_t)codec, (uint8_t)info.channel, (uint8_t)info.bit_per_sample,
                            (uint8_t)sampleRate, (uint8_t)info.channel_count};

    UlucuFrame unibFrame(buffer, frameLen);
    unibFrame.setType('A').setSubType(0);
    unibFrame.setSequence(mSequence[trackAudio]);
    unibFrame.setAudioInfo(frameInfo);
    unibFrame.setDtsPts(frame.dts(), frame.pts());

    outFrame.setSequence(mSequence[trackAudio]);
    mSequence[trackAudio]++;

    uint32_t len = unibFrame.makeFrame((uint8_t*)frame.data(), (uint32_t)frame.size());
    if (len <= 0) {
        errorf("make len <= 0\n");
        return -1;
    }
    outFrame.setMediaFrameType(frame.getMediaFrameType());
    outFrame.setPts(frame.pts());
    outFrame.setAudioFrameInfo(info);
    outFrame.resize(reserveLen + len);
    mMediaFrame[trackAudio] = outFrame;
    return frame.size();
}

int8_t UlucuPack::videoCodec(VideoCodecType codec) {
    switch (codec) {
        case H264:
            return unibVideoCodecH264;
        case H265:
            return unibVideoCodecH265;
        default:
            return unibVideoCodecNone;
    }
}

int8_t UlucuPack::audioCodec(AudioCodecType codec) {
    switch (codec) {
        case AAC:
            return unibAudioCodecAAC;
        case G711a:
            return unibAudioCodecG711a;
        case G711u:
            return unibAudioCodecG711u;
        case G726:
            return unibAudioCodecG726;
        case PCM:
            return unibAudioCodecPCM;
        default:
            return unibAudioCodecNone;
    }
}

/** 采样率转换 */
int8_t UlucuPack::audioSample(uint32_t sample) {
    switch (sample) {
        case 8000:
            return unibSample8000;
        case 11025:
            return unibSample11025;
        case 16000:
            return unibSample16000;
        case 22050:
            return unibSample22050;
        case 32000:
            return unibSample32000;
        case 44100:
            return unibSample44100;
        case 48000:
            return unibSample48000;
        case 64000:
            return unibSample64000;
        case 96000:
            return unibSample96000;
        default:
            return unibSampleNone;
    }
}

MediaFrame UlucuPack::getMediaFrameFromBuffer(const char* buffer, int32_t size) {
    MediaFrame frame;
    return frame;
}
