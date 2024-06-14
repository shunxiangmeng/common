/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaFrame.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-02-25 12:35:34
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "MediaFrame.h"

ObjectStatisticImpl(MediaFrame);

MediaFrame::MediaFrame() : Buffer(0) {
    minternal_ = std::make_shared<Internal>();
}

MediaFrame::MediaFrame(int32_t capacity) : Buffer(capacity) {
    minternal_ = std::make_shared<Internal>();
}

MediaFrame::MediaFrame(const MediaFrame &other) : Buffer(other) {
    minternal_ = other.minternal_;
}

MediaFrame::MediaFrame(MediaFrame&& other) : Buffer(std::move(other)) {
    minternal_ = std::move(other.minternal_);
}

void MediaFrame::operator=(const MediaFrame &other) {
    internal_ = other.internal_;
    minternal_ = other.minternal_;
}

MediaFrame::~MediaFrame() {
}

bool MediaFrame::empty() {
    return Buffer::empty();
}

void MediaFrame::reset() {
    minternal_.reset();
    internal_.reset();
}

bool MediaFrame::isKeyFrame() const {
    return minternal_ && minternal_->type == Video && minternal_->video_info.type == VideoFrame_I;
}

bool MediaFrame::ensureCapacity(int32_t capacity) {
    return infra::Buffer::ensureCapacity(capacity);
}

MediaFrameType MediaFrame::getMediaFrameType() const {
    if (minternal_) {
        return minternal_->type;
    }
    return InvalidFrameType;
}

MediaFrame& MediaFrame::setMediaFrameType(MediaFrameType type) {
    if (minternal_) {
        minternal_->type = type;
    }
    return *this;
}

PlacementType MediaFrame::placementType() const {
    if (minternal_) {
        return minternal_->placement;
    }
    return Annexb;
}

MediaFrame& MediaFrame::setPlacementType(PlacementType placement) {
    if (minternal_) {
        minternal_->placement = placement;
    }
    return *this;
}

static int32_t toLittleEndian(int32_t value) {
    int32_t little = 0;
    uint8_t* p = (uint8_t*)&little;
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
    return little;
}

MediaFrame& MediaFrame::convertPlacementTypeToAnnexb() {
    const int32_t nalu_delimiter_len = 4;
    if (minternal_ && minternal_->placement != Annexb && size() > nalu_delimiter_len) {
        char* buffer = data();
        int32_t remaind_len = size();
        int32_t pos = 0;
        do {
            char* tmp = buffer + pos;
            int32_t* p_nalu_len = (int32_t*)(buffer + pos);
            int32_t nalu_len = toLittleEndian(*p_nalu_len);

            tmp[0] = 0x00;
            tmp[1] = 0x00;
            tmp[2] = 0x00;
            tmp[3] = 0x01;

            pos += nalu_delimiter_len + nalu_len;
            remaind_len -= (nalu_delimiter_len + nalu_len);
        } while (remaind_len > nalu_delimiter_len);
    }
    return *this;
}

MediaFrame& MediaFrame::setVideoFrameInfo(VideoFrameInfo &info) {
    if (minternal_) {
        minternal_->video_info = info;
    }
    return *this;
}

bool MediaFrame::getVideoFrameInfo(VideoFrameInfo &info) {
    if (minternal_) {
        info = minternal_->video_info;
        return true;
    }
    return false;
}

MediaFrame& MediaFrame::setAudioFrameInfo(AudioFrameInfo &info) {
    if (minternal_) {
        minternal_->audio_info = info;
    }
    return *this;
}

bool MediaFrame::getAudioFrameInfo(AudioFrameInfo &info) {
    if (minternal_) {
        info = minternal_->audio_info;
        return true;
    }
    return false;
}

int64_t MediaFrame::pts() const {
    if (minternal_) {
        return minternal_->pts;
    }
    return -1;
}

MediaFrame& MediaFrame::setPts(int64_t pts) {
    if (minternal_) {
        minternal_->pts = pts;
    }
    return *this;
}

int64_t MediaFrame::dts() const {
    if (minternal_) {
        return minternal_->dts;
    }
    return -1;
}

MediaFrame& MediaFrame::setDts(int64_t dts) {
    if (minternal_) {
        minternal_->dts = dts;
    }
    return *this;
}

int32_t MediaFrame::sequence() const {
    if (minternal_) {
        return minternal_->sequence;
    }
    return -1;
}

MediaFrame& MediaFrame::setSequence(int32_t sequence) {
    if (minternal_) {
        minternal_->sequence = sequence;
    }
    return *this;
}
