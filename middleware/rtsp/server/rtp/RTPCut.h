/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RTPCut.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-05 12:13:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include <stdint.h>

namespace RTP {

typedef struct {
    uint8_t* payload;         /**rtp数据的起始地址*/
    uint32_t payloadLen;     /**数据长度*/
    uint8_t  head[16];
    uint8_t  headLen;
}RtpPack;

typedef struct {
    int32_t                 mRtpPackNum;
    std::vector<RtpPack>    mRtpPackVector;
} RtpPackInfo;

/**
 * @brief h264 分割
 * @param packs
 * @param buffer
 * @param size
 * @param rtpLen
 * @return
 */
int32_t rtpCutH264(RtpPackInfo &packs, uint8_t *buffer, uint32_t size, uint32_t rtpLen, bool avcc = false);

/**
 * @brief aac 音频分割
 * @param packs
 * @param buffer
 * @param size
 * @param rtpLen
 * @return
 */
int32_t rtpCutAAC(RtpPackInfo &packs, uint8_t *buffer, uint32_t size, uint32_t rtpLen);

/**
 * @brief h265 分割
 * @param packInfo
 * @param buffer
 * @param size
 * @param rtpLen
 * @return
 */
int32_t rtpCutH265(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpLen);

/**
 * @brief g711 分割
 * @param packInfo
 * @param buffer
 * @param size
 * @param rtpMaxLen
 * @return
 */
int32_t rtpCutG711(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpMaxLen);

}