/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Ulucu.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 16:09:16
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

#define ULUCU_PROTOCOL_VERSION    0x20   /* v2.0 */
#define ULUCU_PROTOCL_HEAD_LEN    20

#pragma pack (1)

/**
 * @brief 私有帧头
 */
typedef struct {
    uint8_t         tag[2];              /**"##" */
    uint8_t         type;                /** @see UNIBType */
    uint8_t         subType;             /** 子类型 */
    uint32_t        payloadLen;          /** 负载长度 */
    uint64_t        dts;                 /** dts UTC时间，单位ms */
    uint16_t        pts;                 /** pts相对dts的偏移值,B帧才需要 */
    uint16_t        exLength;            /** 扩展帧头长度 */
} UlucuFrameHead;   ///20bytes

/**
 * @brief 视频帧扩展头
 */
typedef struct {
    uint8_t     type;                   /** 类型 */
    uint8_t     encode;                 /** 编码类型 */
    uint8_t     rateNum;                /** 帧率 */
    uint8_t     rateDen;
    uint16_t    width;
    uint16_t    height;
} UlucuVideoExHead;

/**
 * @brief 音频扩展长度
 */
typedef struct {
    uint8_t type;                       /** 类型 */
    uint8_t encode;                     /** 编码 */
    uint8_t track;                      /***/
    uint8_t sampleBit;                  /***/
    uint8_t rate;                       /***/
    uint8_t channel;                    /***/
    uint8_t res[2];                     /***/
} UlucuAudioExHead;

/**
 * @brief video frame info
 */
typedef struct {
    uint8_t         encode;            /** 编码类型 */
    uint8_t         type;              /**'I'/'P'/'B'*/
    uint16_t        width;             /** 视频宽 */
    uint16_t        height;            /** 视频高 */
    float           rate;              /** 帧率 */
    uint8_t         res[2];            /** 保留 */
} VFrameInfo;

/**
 * @brief audio frame info
 */
typedef struct {
    uint8_t         encode;             /** 编码类型 */
    uint8_t         track;              /** 编码通道 */
    uint8_t         sampleBit;          /** 编码位数 */
    uint8_t         rate;               /** 采样率 */
    uint8_t         channel;            /** 声道 */
} AFrameInfo;

#pragma pack ()

/**
 * @brief 帧类型
 */
typedef enum {
    invalidFrame = 0,
    videoFrame,
    audioFrame,
    watermark,
    imageFrame,
    auxiliaryFrame,
    hbFrame
} UlucuFrameType;

typedef enum {
    unibVideoCodecNone    = 0,
    unibVideoCodecH264    = 1,
    unibVideoCodecH265    = 2,
    unibVideoCodecMpeg4   = 3,
} UlucuVideoCodec;

/**
 * @brief 音频编码
 */
typedef enum {
    unibAudioCodecNone      = 0,
    unibAudioCodecPCM       = 1,
    unibAudioCodecADPCM     = 2,
    unibAudioCodecG711a     = 3,
    unibAudioCodecG711u     = 4,
    unibAudioCodecG722      = 5,
    unibAudioCodecG7221     = 6,
    unibAudioCodecG726      = 7,
    unibAudioCodecAAC       = 8
} UlucuAudioCodec;

/**
 * @brief 音频采样率
 */
typedef enum {
    unibSampleNone,
    unibSample4000,
    unibSample8000,
    unibSample11025,
    unibSample16000,
    unibSample20000,
    unibSample22050,
    unibSample32000,
    unibSample44100,
    unibSample48000,
    unibSample64000,
    unibSample96000,
    unibSample128000,
    unibSample192000,
} UlucuAudioSample;
