/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  vsdp.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-21 15:16:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <queue>
#include <functional>
#include "infra/include/thread/Thread.h"
#include "common/mediafiles/mp4/MP4Reader.h"

class VirtualDSP : public infra::Thread {
public:
    VirtualDSP();
    ~VirtualDSP();

    bool initial(const char* file_name);
    bool deInitial();

private:
    virtual void run() override;

private:
    bool init_;
    MP4Reader mp4_reader_;
    std::queue<MediaFrame> video_frame_queue_;
    std::queue<MediaFrame> audio_frame_queue_;
};
