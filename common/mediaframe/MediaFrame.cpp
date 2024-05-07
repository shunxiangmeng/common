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
