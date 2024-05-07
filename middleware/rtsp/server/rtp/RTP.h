/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RTP.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-05 13:14:26
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>
#pragma pack(push)
#pragma pack(1)

/**
 * @brief rtp over tcp ext header
 */
typedef struct {
    uint8_t     dollar;
    uint8_t     channel;
    uint16_t    len;
} RtpTcpHdr;

/**
 * @brief common rtp header
 */
typedef struct {
    uint8_t     vpxcc;
    uint8_t     mpt;
    uint16_t    sequence;
    uint32_t    timestamp;
    uint32_t    ssrc;
} RtpHdr;

#pragma pack(pop)

