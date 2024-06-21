/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UlucuPlatform.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-17 19:29:35
 * Description :  anyan and huidian 的结合
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <stdint.h>

namespace platform {

typedef struct
{
    int       block_nums;       /* 开辟的缓冲区的数量单位是8k*block_nums */
    uint8_t   channel_num;      /* 通道数量 */
    uint16_t  max_rate;         /* 最大码率 */
    uint16_t  min_rate;         /* 最小码率 */
    char      ptz_ctrl;         /* 是否支持ptz控制 0 不支持,1 支持上下左右 2 支持上下左右和变焦,3 支持上下左右和预置位, 4 支持上下左右、变焦和预置位 */
    char      dev_type;         /* 设备类型0 未知 1 dvr, 2 nvr, 3 ipc, 4 box */
    char      mic_flag;         /* 是否有拾音器 0 没有, 1 有 */
    char      can_rec_voice;    /* 可以接受音频 0 不支持, 1 支持 */
    char      hard_disk;        /* 是否有本地存储 0 没有, 1 有硬盘 */
    char      *p_rw_path;       /* 可读写路径,存放sn,库文件等 */
    char      audio_type;       /* 音频类型 0:aac, 1:g711_a, 2:g711_u, 3:mp3, 4:adpcm,5:G726 */
    char      audio_chnl;       /* 音频通道,单通道 1, 双通道 2 */
    uint16_t  audio_smaple_rt;  /* 音频采样率 */
    uint16_t  audio_bit_width;  /* 位宽 */

    uint8_t   use_type;         /* 设备使用类型：0：对外销售设备，1：测试设备，2：演示设备。 默认0 */
    uint8_t   has_tfcard;       /* 是否有TF卡 0: 没有 1:有 */
    uint8_t   video_type;       /* 视频类型 0: H264, 1: HEVC(H265) */
    uint8_t   lan_disable;      /* 禁用局域网功能 0: 启用(default), 1:禁用(disable) */
    uint8_t   Reserved[16];     /* 预留字段, Reserved[0]: max_stream_num_per_chn*/
} DeviceAttribute;

class IPlatform {
public:

    static std::shared_ptr<IPlatform> create();

    virtual bool init(DeviceAttribute *device_attribute) = 0;

};


}