/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UlucuFrame.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 16:15:49
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string.h>
#include "Ulucu.h"
#include "UlucuFrame.h"

static int32_t toLittleEndian(int32_t value) {
    int32_t little = 0;
    uint8_t* p = (uint8_t*)&little;
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
    return little;
}

UlucuFrame::UlucuFrame(uint8_t *buffer,uint32_t size) : mBuffer(buffer), mBufferSize(size) {
}

UlucuFrame::~UlucuFrame() {
}

UlucuFrame& UlucuFrame::setSequence(uint32_t seq) {
    if (mBuffer == nullptr) {
        return *this;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return *this;
}

UlucuFrame& UlucuFrame::getSequence(uint32_t &seq) {
    if (mBuffer == nullptr) {
        return *this;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return *this;
}

UlucuFrame& UlucuFrame::setDtsPts(uint64_t dts, uint64_t pts) {
    if (mBuffer == nullptr) {
        return *this;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    head->dts = dts;
    head->pts = uint16_t(pts - dts);
    return *this;
}

uint64_t UlucuFrame::dts() {
    if (mBuffer == nullptr) {
        return 0;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return head->dts;
}

uint64_t UlucuFrame::pts() {
    if (mBuffer == nullptr) {
        return 0;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return head->dts + head->pts;
}

UlucuFrame& UlucuFrame::setType(uint8_t type) {
    if (mBuffer == nullptr) {
        return *this;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    switch(type) {
        case 'V': head->type = videoFrame;
            break;
        case 'A': head->type = audioFrame;
            break;
        case 'J': head->type = imageFrame;
            break;
        default:
            break;
    }
    return *this;
}

uint8_t UlucuFrame::getType() {
    if (mBuffer == nullptr) {
        return 0;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return head->type;
}

UlucuFrame& UlucuFrame::setSubType(uint8_t type) {
    if (mBuffer == nullptr) {
        return *this;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    head->subType = type;
    return *this;
}

uint8_t UlucuFrame::getSubType() {
    if (mBuffer == nullptr) {
        return 0;
    }
    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    return head->subType;
}

UlucuFrame& UlucuFrame::setVideoInfo(const VFrameInfo &info) {
    if (mBuffer == nullptr) {
        return *this;
    }

    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    if (head->type != videoFrame || head->subType != 'I') {
        return *this;
    }

    UlucuVideoExHead *videoEx = reinterpret_cast<UlucuVideoExHead*>(mBuffer + sizeof(UlucuFrameHead));
    videoEx->type = videoFrame;
    videoEx->encode = info.encode;
    if (info.rate < 1 && info.rate > 0) {
        int32_t den = int32_t(1.0f / info.rate);
        videoEx->rateNum = 1;
        videoEx->rateDen = uint8_t(den);
    } else {
        videoEx->rateNum = uint8_t(info.rate);
        videoEx->rateDen = 1;
    }

    videoEx->width  = info.width;
    videoEx->height = info.height;
    return *this;
}

UlucuFrame& UlucuFrame::setAudioInfo(const AFrameInfo &info) {
    if (mBuffer == nullptr) {
        return *this;
    }

    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);
    if (head->type != audioFrame) {
        return *this;
    }

    UlucuAudioExHead *audioEx = reinterpret_cast<UlucuAudioExHead*>(mBuffer + sizeof(UlucuFrameHead));
    audioEx->type = audioFrame;
    audioEx->encode = info.encode;
    audioEx->track = info.track;
    audioEx->sampleBit = info.sampleBit;
    audioEx->rate = info.rate;
    audioEx->channel = info.channel;
    return *this;
}

uint32_t UlucuFrame::makeFrame(uint8_t *buffer, uint32_t len, bool avcchvcc) {
    if (mBuffer == nullptr) {
        return 0;
    }

    UlucuFrameHead *head = reinterpret_cast<UlucuFrameHead*>(mBuffer);

    head->tag[0] = '#';
    head->tag[1] = '#';

    head->payloadLen = len;
    head->exLength = 0;

    uint8_t* to = mBuffer + sizeof(UlucuFrameHead);
    uint32_t total = sizeof(UlucuFrameHead);

    if (head->type == videoFrame && head->subType == 'I') {
        head->exLength = sizeof(UlucuVideoExHead);
        to += sizeof(UlucuVideoExHead);
        total += sizeof(UlucuVideoExHead);
    } else if (head->type == audioFrame) {
        head->exLength = sizeof(UlucuAudioExHead);
        to += sizeof(UlucuAudioExHead);
        total += sizeof(UlucuAudioExHead);
    }

    memcpy(to, buffer, len);
    if (avcchvcc) {
        int32_t pos = 0;
        int32_t remaind_len = len;
        do {
            int32_t *p_nalu_len = (int32_t*)(to + pos);
            int32_t nalu_len = toLittleEndian(*p_nalu_len);
            to[pos + 0] = 0x00;     //avcc_hvcc to annexb
            to[pos + 1] = 0x00;
            to[pos + 2] = 0x00;
            to[pos + 3] = 0x01;
            pos += 4 + nalu_len;
            remaind_len -= (4 + nalu_len);
        } while (remaind_len > 4);
    }

    to += len;
    total += len;
    return total;
}
