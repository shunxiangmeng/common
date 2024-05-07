/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  MediaSource.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 12:34:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once

typedef enum {
    LiveStream,
    RecordStream,
    RemoteStream,
    FileStream,
} StreamType;

class MediaSource {
    MediaSource() = default;
    ~MediaSource() = default;
public:
    static MediaSource* instance();
};