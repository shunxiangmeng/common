/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaFrameList.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-08 17:59:28
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "MediaFrameList.h"

MediaFrameList::MediaFrameList() {
}

MediaFrameList::~MediaFrameList() {
}

void MediaFrameList::push_back(MediaFrame frame) {
    std::lock_guard<std::mutex> guard(mutex_);
    list_.push_back(frame);
}

void MediaFrameList::push_front(MediaFrame frame) {
    std::lock_guard<std::mutex> guard(mutex_);
    list_.push_front(frame);
}

void MediaFrameList::pop_front() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (list_.size() == 0) {
        return;
    }
    list_.pop_front();
}

void MediaFrameList::pop_back() {
    std::lock_guard<std::mutex> guard(mutex_);
    list_.pop_back();
}

int32_t MediaFrameList::size() const {
    return (int32_t)list_.size();
}

bool MediaFrameList::empty() const {
    return list_.size() == 0;
}

MediaFrame MediaFrameList::front() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (list_.size() == 0) {
        return MediaFrame();
    }
    return list_.front();
}

MediaFrame MediaFrameList::take_front() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (list_.size() == 0) {
        return MediaFrame();
    }
    MediaFrame frame = list_.front();
    list_.pop_front();
    return frame;
}