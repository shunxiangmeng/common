/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaFrameList.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-08 17:59:01
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <list>
#include <mutex>
#include <algorithm>
#include "MediaFrame.h"

class MediaFrameList {
public:
    MediaFrameList();
    ~MediaFrameList();

    void push_back(MediaFrame frame);
    void push_front(MediaFrame frame);

    void pop_front();
    void pop_back();

    int32_t size() const;
    bool empty() const;

    MediaFrame front();
    MediaFrame take_front();

private:
    std::list<MediaFrame> list_;
    std::mutex mutex_;
};