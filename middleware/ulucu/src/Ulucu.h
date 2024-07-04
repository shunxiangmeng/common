/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Ulucu.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-22 13:43:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include "ulucu/include/IUlucu.h"
#include "huidian/Huidian.h"
#include "anyan/src/inc/Anyan_Device_SDK.h"
#include "stream/mediasession/MediaSession.h"

namespace ulucu {

class Ulucu : public IUlucu {
public:
    Ulucu();
    ~Ulucu();

    virtual bool init() override;

    static Ulucu* instance();

    void anyanInteractCallback(void* args);
private:
    void initAnyan(const char* sn);
    void initHuidian();
    void parseServiceList(infra::Buffer& buffer);
    void playVideo(bool start, int32_t channel, int32_t bitrate);
    void playAudio(bool start, int32_t channel);

    int32_t bitrateToSubchannel(int32_t bitrate);
    int32_t subchannelToBitrate(int32_t sub_channel);
    void onMediaData(int32_t channel, int32_t sub_channel, MediaFrameType type, MediaFrame &frame);

private:
    std::shared_ptr<Huidian> huidian_;
    std::string device_sn_;

    std::shared_ptr<MediaSession> media_sessions_[3];

    std::string domain_public_api_service_;
    std::unordered_map<const char*, std::string> service_domain_map_;

};

}