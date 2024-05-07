/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RTPCut.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-05 12:14:37
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "RTPCut.h"
#include "infra/include/Logger.h"

namespace RTP {

#define NALU_DELIMITER_LEN     4

#define H264_NAL_UNIT_UNDEF    0
#define H264_NAL_UNIT_SLICE    1
#define H264_NAL_UNIT_IDR      5
#define H264_NAL_UNIT_SEI      6
#define H264_NAL_UNIT_SPS      7
#define H264_NAL_UNIT_PPS      8
#define H264_NAL_UNIT_AUD      9

#define H265_NAL_UNIT_SLICE    0x01
#define H265_NAL_UNIT_IDR      0x13

static int32_t cutH264Slice(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpLen);
static int32_t cutH264SliceFua(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpLen);

static int32_t cutH265Slice(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpMaxLen);
static int32_t cutH265SliceFua(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpMaxLen);

static int32_t toLittleEndian(int32_t value) {
    int32_t little = 0;
    uint8_t* p = (uint8_t*)&little;
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
    return little;
}

int32_t rtpCutH264Avcc(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpLen) {
    int32_t remaind_len = size;
    int32_t pos = 0;
    bool has_sps = false;
    bool has_pps = false;
    do {
        int32_t *p_nalu_len = (int32_t*)(buffer + pos);
        int32_t nalu_len = toLittleEndian(*p_nalu_len);
        uint8_t nalu_type = buffer[pos + NALU_DELIMITER_LEN] & 0x1f;  // low 5 bit
        switch (nalu_type) {
            case H264_NAL_UNIT_SEI:
            case H264_NAL_UNIT_AUD:
                // discard this nalu
                break;
            case H264_NAL_UNIT_SPS:
                if (!has_sps) {
                    has_sps = true;
                    cutH264Slice(packInfo, buffer + pos + NALU_DELIMITER_LEN, nalu_len, rtpLen);
                }
                break;
            case H264_NAL_UNIT_PPS:
                if (!has_pps) {
                    has_pps = true;
                    cutH264Slice(packInfo, buffer + pos + NALU_DELIMITER_LEN, nalu_len, rtpLen); 
                }
                break;
            default:
                cutH264Slice(packInfo, buffer + pos + NALU_DELIMITER_LEN, nalu_len, rtpLen);
                break;
        }
        pos += NALU_DELIMITER_LEN + nalu_len;
        remaind_len -= (NALU_DELIMITER_LEN + nalu_len);
    } while (remaind_len > NALU_DELIMITER_LEN);
    return size;
}

int32_t rtpCutH264(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpLen, bool avcc) {
    if (avcc) {
        return rtpCutH264Avcc(packInfo, buffer, size, rtpLen);
    }

    int32_t pos = -1;
    uint8_t *slice = nullptr;
    int32_t sliceLen = 0;
    for (auto index = 0; index < (size - NALU_DELIMITER_LEN); index++) {
        if (buffer[index + 0] == 0x00 && buffer[index + 1] == 0x00 && buffer[index + 2] == 0x00 && buffer[index + 3] == 0x01) {
            uint8_t nalu_type = buffer[index + NALU_DELIMITER_LEN] & 0x1f;  // low 5 bit
            if (pos > 0) {
                slice = buffer + pos;
                sliceLen = index - pos;
                cutH264Slice(packInfo, slice, sliceLen, rtpLen);
                if (nalu_type == H264_NAL_UNIT_IDR ||  nalu_type == H264_NAL_UNIT_SLICE) {
                    slice = buffer + index + NALU_DELIMITER_LEN;
                    sliceLen = size - index - NALU_DELIMITER_LEN;
                    cutH264Slice(packInfo, slice, sliceLen, rtpLen);
                    break;
                }
            } else if (nalu_type == H264_NAL_UNIT_IDR || nalu_type == H264_NAL_UNIT_SLICE) {
                slice = buffer + index + NALU_DELIMITER_LEN;
                sliceLen = size - index - NALU_DELIMITER_LEN;
                cutH264Slice(packInfo, slice, sliceLen, rtpLen);
                break;
            }
            pos = index + NALU_DELIMITER_LEN;
        } else if (index == (size - NALU_DELIMITER_LEN)) {
            /**TOOD*/
            errorf("rtp pack todo...\n");
        }
    }
    return size;
}

int32_t rtpCutH265(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpMaxLen) {
    int32_t pos = -1;
    uint8_t *slice = nullptr;
    int32_t sliceLen = 0;
    for (auto index = 0U; index < (size - NALU_DELIMITER_LEN); index++) {
        if (buffer[index + 0] == 0x00 && buffer[index + 1] == 0x00 && buffer[index + 2] == 0x00 && buffer[index + 3] == 0x01) {
            uint8_t nalutype = (buffer[index + NALU_DELIMITER_LEN] & 0x7E) >> 1; //中间6bit
            if (pos > 0) {
                slice = buffer + pos;
                sliceLen = index - pos;
                cutH265Slice(packInfo, slice, sliceLen, rtpMaxLen);
                if (nalutype == H265_NAL_UNIT_IDR || nalutype == H265_NAL_UNIT_SLICE) {
                    slice = buffer + index + NALU_DELIMITER_LEN;
                    sliceLen = size - index - NALU_DELIMITER_LEN;
                    cutH265Slice(packInfo, slice, sliceLen, rtpMaxLen);
                    break;
                }
            } else if (nalutype == H265_NAL_UNIT_IDR || nalutype == H265_NAL_UNIT_SLICE) {
                slice = buffer + index + NALU_DELIMITER_LEN;
                sliceLen = size - index - NALU_DELIMITER_LEN;
                cutH265Slice(packInfo, slice, sliceLen, rtpMaxLen);
                break;    
            }
            pos = index + NALU_DELIMITER_LEN;
        } else if (index == (size - NALU_DELIMITER_LEN)) {
            /*TOOD...*/
            errorf("rtp pack todo...\n");
        }
    }
    return size;
}

int32_t rtpCutAAC(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpMaxLen) {
    if (buffer == nullptr) {
        return -1;
    }
    /** ADTS头不能打包进payload */
    size -= 7;
    buffer += 7;
    int32_t packetNum = (size + rtpMaxLen - 1) / rtpMaxLen;
    for (auto i = 0; i < packetNum; i++) {
        int packlen = ((i == packetNum - 1) ? (size - i * rtpMaxLen) : rtpMaxLen);
        int32_t pos = packInfo.mRtpPackNum;
        if (pos >= packInfo.mRtpPackVector.size()) {
            ///扩展vector
            RtpPack pack;
            pack.headLen = 4;
            pack.head[0] = 0x00;
            pack.head[1] = 0x10;
            pack.head[2] = size >> 5;
            pack.head[3] = (size & 0x1F) << 3;
            pack.payload = buffer + i * rtpMaxLen;
            pack.payloadLen = packlen;
            packInfo.mRtpPackVector.push_back(pack);
        } else {
            packInfo.mRtpPackVector[pos].headLen = 4;
            packInfo.mRtpPackVector[pos].head[0] = 0x00;
            packInfo.mRtpPackVector[pos].head[1] = 0x10;
            packInfo.mRtpPackVector[pos].head[2] = size >> 5;
            packInfo.mRtpPackVector[pos].head[3] = (size & 0x1F) << 3;
            packInfo.mRtpPackVector[pos].payload = buffer + i * rtpMaxLen;
            packInfo.mRtpPackVector[pos].payloadLen = packlen;
        }
        packInfo.mRtpPackNum++;
    }
    return size;
}

int32_t rtpCutG711(RtpPackInfo &packInfo, uint8_t *buffer, uint32_t size, uint32_t rtpMaxLen) {
    if (buffer == nullptr){
        return -1;
    }
    int32_t packetNum = (size + rtpMaxLen - 1) / rtpMaxLen;
    for (auto i = 0L; i < packetNum; i++) {
        int packlen = ((i == packetNum - 1) ? (size - i * rtpMaxLen) : rtpMaxLen);
        uint32_t pos = packInfo.mRtpPackNum;
        if (pos >= packInfo.mRtpPackVector.size()) {
            ///扩展vector
            RtpPack pack;
            pack.headLen = 0;
            pack.payload = buffer + i * rtpMaxLen;
            pack.payloadLen = packlen;
            packInfo.mRtpPackVector.push_back(pack);
        } else {
            packInfo.mRtpPackVector[pos].headLen = 0;
            packInfo.mRtpPackVector[pos].payload = buffer + i * rtpMaxLen;
            packInfo.mRtpPackVector[pos].payloadLen = packlen;
        }
        packInfo.mRtpPackNum++;
    }
    return size;
}

static int32_t cutH264SliceFua(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpLen) {
    if (buffer == nullptr || size <= 0){
        return 0;
    }
    uint8_t header = buffer[0];
    uint8_t type = header & 0x1F;
    uint8_t fuaHeader = (header & 0xE0) | 28;
    uint8_t fuaHeader2 = 0;
    uint8_t *naluData = buffer + 1;
    int32_t naluDataLen = size -1;
    int32_t fuaLen = rtpLen - 2;
    int32_t fuaNum = (naluDataLen + fuaLen - 1) / fuaLen;
    for (auto i = 0; i < fuaNum; i++) {
        uint8_t *fuaData = naluData + i * fuaLen;
        int32_t fuaDataLen = fuaLen;
        fuaHeader2 = type;
        if (i == fuaNum - 1) {
            fuaDataLen = naluDataLen - i * fuaLen;
        }

        if (i == 0) {
            /**第一个fua*/
            fuaHeader2 |= 0x80;
        } else if (i == fuaNum - 1) {
            /**最后一个fua*/
            fuaHeader2 |= 0x40;
        }

        int32_t pos = packInfo.mRtpPackNum;
        if (pos >= packInfo.mRtpPackVector.size()) {
            /**扩展vector*/
            RtpPack pack;
            pack.headLen = 2;
            pack.head[0] = fuaHeader;pack.head[1] = fuaHeader2;
            pack.payload = fuaData;pack.payloadLen = fuaDataLen;
            packInfo.mRtpPackVector.push_back(pack);
        } else {
            packInfo.mRtpPackVector[pos].headLen = 2;
            packInfo.mRtpPackVector[pos].head[0] = fuaHeader;
            packInfo.mRtpPackVector[pos].head[1] = fuaHeader2;
            packInfo.mRtpPackVector[pos].payload = fuaData;
            packInfo.mRtpPackVector[pos].payloadLen = fuaDataLen;
        }
        packInfo.mRtpPackNum++;
    }
    return size;
}

static int32_t cutH264Slice(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpLen) {
    if (buffer == nullptr || size <= 0){
        return 0;
    }
    if (size > rtpLen){
        return cutH264SliceFua(packInfo, buffer, size, rtpLen);
    }
    int32_t pos = packInfo.mRtpPackNum;
    if (pos >= packInfo.mRtpPackVector.size()){
        /**扩展vector*/
        RtpPack pack;
        pack.headLen = 0;pack.payload = buffer;pack.payloadLen = size;
        packInfo.mRtpPackVector.push_back(pack);
    } else {
        packInfo.mRtpPackVector[pos].headLen = 0;
        packInfo.mRtpPackVector[pos].payload = buffer;
        packInfo.mRtpPackVector[pos].payloadLen = size;
    }
    packInfo.mRtpPackNum++;
    return size;
}

static int32_t cutH265SliceFua(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpMaxLen) {
    if (buffer == nullptr || size <= 0){
        return 0;
    }
    /*
    h265的nalutype是起始码后的两个字节, type = (buffer[0]&0x7E)>>1;
    +---------------+---------------+
    |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |F|   Type    |  LayerId  | TID |
    +---------------+---------------+
    */
    uint8_t type = (buffer[0] & 0x7E) >> 1;
    uint8_t fuaHeader0 = (buffer[0] & 0x81) | (49/*H265_FRAGMENT_UNIT_TYPE*/ << 1);
    uint8_t fuaHeader1 = buffer[1];
    uint8_t fuaHeader2 = 0;
    /*
    *    see chapter 4.8 of draft-ietf-payload-rtp-h265-06.pdf
    *    create the FU header
    *    0               7        
    *    +-+-+-+-+-+-+-+-+
    *    |S|E|  FuType   |
    *    +-+-+-+-+-+-+-+-+
    *
    *    S       = variable
    *    E       = variable
    *    FuType  = NAL unit type
    */
    int32_t fuaLen = rtpMaxLen - 3;
    uint8_t *naluData = buffer + 2;
    int32_t naluDataLen = size - 2;
    int32_t fuaNum = (naluDataLen + fuaLen - 1) / fuaLen;
    for (auto i = 0; i < fuaNum; i++) { 
        uint8_t *fuaData = naluData + i * fuaLen;
        int32_t fuaDataLen = fuaLen;    
        fuaHeader2 = type;
        if (i == fuaNum - 1) {
            fuaDataLen = naluDataLen - i * fuaLen;
        }    
        if (i == 0) {
            /**第一个fua*/
            fuaHeader2 |= 0x80;
        } else if (i == fuaNum - 1) {
            /**最后一个fua*/
            fuaHeader2 |= 0x40;
        } else {
            fuaHeader2 |= 0x00;
        }
        uint32_t pos = packInfo.mRtpPackNum;
        if (pos >= packInfo.mRtpPackVector.size()) {
            /**扩展vector*/
            RtpPack pack;
            pack.headLen = 3;
            pack.head[0] = fuaHeader0;
            pack.head[1] = fuaHeader1;
            pack.head[2] = fuaHeader2;
            pack.payload = fuaData;
            pack.payloadLen = fuaDataLen;
            packInfo.mRtpPackVector.push_back(pack);
        } else {
            packInfo.mRtpPackVector[pos].headLen = 3;
            packInfo.mRtpPackVector[pos].head[0] = fuaHeader0;
            packInfo.mRtpPackVector[pos].head[1] = fuaHeader1;
            packInfo.mRtpPackVector[pos].head[2] = fuaHeader2;
            packInfo.mRtpPackVector[pos].payload = fuaData;
            packInfo.mRtpPackVector[pos].payloadLen = fuaDataLen;
        }
        packInfo.mRtpPackNum++;        
    }
    return size;
}

static int32_t cutH265Slice(RtpPackInfo &packInfo, uint8_t *buffer, int32_t size, int32_t rtpMaxLen) {
    if (buffer == nullptr || size <= 0) {
        return 0;
    }
    if (size > rtpMaxLen) {
        return cutH265SliceFua(packInfo, buffer, size, rtpMaxLen);
    } 
    uint32_t pos = packInfo.mRtpPackNum;
    if (pos >= packInfo.mRtpPackVector.size()) {
        /**扩展vector*/
        RtpPack pack;
        pack.headLen = 0;
        pack.payload = buffer;
        pack.payloadLen = size;
        packInfo.mRtpPackVector.push_back(pack);
    } else {
        packInfo.mRtpPackVector[pos].headLen = 0;
        packInfo.mRtpPackVector[pos].payload = buffer;
        packInfo.mRtpPackVector[pos].payloadLen = size;
    }
    packInfo.mRtpPackNum++;
    return size;       
}

}