/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Ulucu.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-22 13:43:35
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Ulucu.h"
#include "anyan/src/inc/Anyan_Device_SDK.h"
#include "infra/include/Logger.h"
#include "infra/include/thread/WorkThreadPool.h"

namespace ulucu {

IUlucu* IUlucu::instance() {
    return Ulucu::instance();
}

Ulucu* Ulucu::instance() {
    static Ulucu s_ulucu;
    return &s_ulucu;
}

Ulucu::Ulucu() {
    huidian_ = std::make_shared<Huidian>();
}

Ulucu::~Ulucu() {
}

static void anyan_interact_callback(CMD_PARAM_STRUCT *args) {
    Ulucu::instance()->anyanInteractCallback((void*)args);
}

void Ulucu::anyanInteractCallback(void* data) {
    CMD_PARAM_STRUCT *args = (CMD_PARAM_STRUCT*)data;
    warnf("anyan cmd: %d\n", args->cmd_id);
    switch (args->cmd_id) {
        case VIDEO_CTRL: {
            bool start = args->cmd_args[0];
            int32_t bitrate = args->cmd_args[2] * 256 + (uint8_t)args->cmd_args[1];
            tracef("%s video channel:%d, rate:%d\n", args->cmd_args[0] ? "start" : "stop", args->channel, bitrate);
            playVideo(args->cmd_args[0], args->channel, bitrate);
        }
        break;
        case HISTORY_CTRL:
            tracef("anyan cmd history_ctrl\n");
            break;
        case AUDIO_CTRL:
            tracef("anyan cmd audio_ctrl\n");
            break;
        case TALKING_CTRL:
            tracef("anyan cmd talking_ctrl\n");
            break;
        case PTZ_CTRL:
            tracef("anyan cmd ptz_ctrl\n");
            break;
        case PTZ_SET_PRE_LOCAL:
            tracef("anyan cmd ptz_set_pre_local\n");
            break;
        case PTZ_CALL_PRE_LOCAL:
            tracef("anyan cmd ptz_call_pre_local\n");
            break;
        case ALARM_CTRL:
            tracef("anyan cmd alarm_local\n");
            break;
        case TIME_SYN:
            tracef("anyan cmd time_syn\n");
            break;
        case EXT_DEVICE_ONLINE:
            infof("anyan online\n");
            break;
        default:
            warnf("undefine cmd [%d]\n", args->cmd_id);
            break; 
    }
}

bool Ulucu::init() {

    Dev_SN_Info oem_info = {0};
    oem_info.OEMID = 100002;
    //char *SN = "Ub0000000123456444NN";
    //char* SN = "Ub0000000000614053dd";
    char* SN = "Ub0000000000589849TD";
    device_sn_ = SN;
    memcpy(oem_info.MAC, "0A0027000004", sizeof("0A0027000004"));
    memcpy(oem_info.SN, SN, strlen(SN));
    strcpy(oem_info.OEM_name, "Ub");
    strcpy(oem_info.Factory, "ULUCU");
    strcpy(oem_info.Model, "UL-LAPI08");

    //检测网络连接 
    Dev_Attribut_Struct attr = {0};
    attr.block_nums = 200;  // 流缓冲区 200 * 8k
    attr.channel_num = 3;   // 设备通道数
    attr.hard_disk = 0;
    attr.p_rw_path = (char*)".";  //可读写目录路径
    attr.lan_disable = 0;
    attr.ptz_ctrl = 3;
    attr.dev_type = 3;      /* 设备类型0 未知 1 dvr, 2 nvr, 3 ipc, 4 box */
    attr.mic_flag = 0;
    attr.can_rec_voice = 0;
    attr.video_type = 0;
    attr.audio_type = 0;
    attr.audio_chnl = 1;
    attr.audio_smaple_rt = 8000;
    attr.audio_bit_width = 16;
    attr.use_type = 0;      /* 设备使用类型 0：对外销售设备，1：测试设备，2：演示设备。 默认0 */
    attr.max_rate = UPLOAD_RATE_4;
    attr.min_rate = UPLOAD_RATE_3;

    int ret = Ulu_SDK_Init_All(&oem_info, &attr, NULL);
    Ulu_SDK_Set_Interact_CallBack(anyan_interact_callback);

    initHuidian();

    return true;
}

void Ulucu::initHuidian() {
    infra::WorkThreadPool::instance()->async([this]() {
        huidian_->init(device_sn_.data());
    });
    
}

int32_t Ulucu::bitrateToSubchannel(int32_t bitrate) {
    if (bitrate >= UPLOAD_RATE_4) {
        return 0;
    } else if (bitrate >= UPLOAD_RATE_3) {
        return 1;
    } else if (bitrate >= UPLOAD_RATE_2) {
        return 2;
    } else if (bitrate >= UPLOAD_RATE_1) {
        return 3;
    } else {
        return 4;
    }
}

int32_t Ulucu::subchannelToBitrate(int32_t sub_channel) {
    switch (sub_channel) {
        case 0: return UPLOAD_RATE_4;
        case 1: return UPLOAD_RATE_3;
        case 2: return UPLOAD_RATE_2;
        case 3: return UPLOAD_RATE_1;
        default: return UPLOAD_RATE_4;
    }
}

void Ulucu::playVideo(bool start, int32_t channel, int32_t bitrate) {
    int32_t subchannel = bitrateToSubchannel(bitrate);
    if (start) {
        if (!media_sessions_[subchannel]) {
            media_sessions_[subchannel] = std::make_shared<MediaSession>(0, subchannel);
        }
        media_sessions_[subchannel]->create();
        if (!media_sessions_[subchannel]->start(MediaSession::OnFrameProc(&Ulucu::onMediaData, this))) {
            errorf("start streaming error\n");     
        }
    } else {
        if (media_sessions_[subchannel]) {
            if (!media_sessions_[subchannel]->stop(MediaSession::OnFrameProc(&Ulucu::onMediaData, this))) {
                errorf("stop streaming error\n");
            }
        }
    }
}

void Ulucu::onMediaData(int32_t channel, int32_t sub_channel, MediaFrameType type, MediaFrame &frame) {
    if (type == Audio) {
        return;
    }
    if (type == Video) {
        frame.convertPlacementTypeToAnnexb();
    }
    //tracef("ondata sub_channel:%d, type:%d, size:%d\n", sub_channel, type, frame.size());
    Stream_Event_Struct stream_event = {0};
    stream_event.channelnum = channel + 1;
    stream_event.bit_rate = subchannelToBitrate(sub_channel);
    if (type == Audio) {
        AudioFrameInfo info;
        frame.getAudioFrameInfo(info);
        int32_t ay_audio_codec = Media_Audio_G711A;
        if (info.codec == AudioCodecType::AAC) {
            ay_audio_codec = Media_Audio_AAC;
        } else if (info.codec == AudioCodecType::G711a) {
            ay_audio_codec = Media_Audio_G711A;
        } else if (info.codec == AudioCodecType::G711u) {
            ay_audio_codec = Media_Audio_G711U;
        } else {
            errorf("not support audio codec:%d\n", info.codec);
            return;
        }
        stream_event.frm_type = CH_AUDIO_FRM;
        stream_event.media_audio_type = ay_audio_codec;
    } else {
        VideoFrameInfo info;
        frame.getVideoFrameInfo(info);
        int32_t ay_video_codec = Media_Video_H264;
        if (info.codec == VideoCodecType::H264) {
            ay_video_codec = Media_Video_H264;
        } else if (info.codec == VideoCodecType::H265) {
            ay_video_codec = Media_Video_H265;
        } else {
            errorf("not support video codec:%d\n", info.codec);
            return;
        }
        stream_event.frm_type = CH_P_FRM;
        stream_event.media_video_type = ay_video_codec;
        if (frame.isKeyFrame()) {
            stream_event.frm_type = CH_I_FRM;
        }
    }
    stream_event.frm_id = 0;
    stream_event.frm_av_id = 0;

    stream_event.frm_ts = (uint32_t)frame.dts();
    stream_event.pdata = frame.data();
    stream_event.frm_size = frame.size();

    int ret = Ulu_SDK_Stream_Event_Report(&stream_event);
    if (ret < 0) {
        errorf("Ulu_SDK_Stream_Event_Report error, ret:%d\n", ret);
    }
}

}