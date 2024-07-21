/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  vdsp.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-21 15:16:33
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "vdsp/include/vsdp.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"
#include "hal/Video.h"

VirtualDSP::VirtualDSP() : init_(false) {
}

VirtualDSP::~VirtualDSP() {
}

bool VirtualDSP::initial(const char* file_name) {
    if (!init_) {
        init_ = true;
        //std::string filename = "F:\\mp4\\The.Teacher.2022.HD1080P.X264.AAC.Malayalam.CHS.BDYS.mp4";
        //std::string filename = "/home/shawn/test.mp4";
        //std::string filename = "F:\\mp4\\HWZ.2022.EP01.HD1080P.X264.AAC.Mandarin.CHS.BDYS.mp4";
        std::string filename = file_name;
        mp4_reader_.open(filename);
        Thread::start();
    }
    return true;
}

bool VirtualDSP::deInitial() {
    return false;
}

void VirtualDSP::run() {
    infof("VirtualDSP thread start\n");
    MediaFrame frame;
    while (running()) {
        int64_t now = infra::getCurrentTimeMs();
        if (!video_frame_queue_.empty()) {
            frame = video_frame_queue_.front();
            if (frame.dts() <= now) {
                hal::IVideo::instance()->sendFrameVdsp(frame);
                video_frame_queue_.pop();
            }
        }

        if (video_frame_queue_.size() > 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        frame = mp4_reader_.getFrame();
        if (!frame.empty()) {
            if (frame.getMediaFrameType() == Video) {
                frame.convertPlacementTypeToAnnexb();
                frame.setPlacementType(Annexb);
                video_frame_queue_.push(frame);
                //infof("video queue size:%d\n", video_frame_queue_.size());
            }
        } else {
            int64_t timestamp = 0;
            //mp4_reader_.seek(&timestamp);
            //warnf("seek to %lld\n", timestamp);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    warnf("VirtualDSP thread exit\n");
}