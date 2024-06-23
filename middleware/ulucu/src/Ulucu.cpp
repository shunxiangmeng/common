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

namespace ulucu {

IUlucu* IUlucu::instance() {
    static Ulucu s_ulucu;
    return &s_ulucu;
}

Ulucu::Ulucu() {
}

Ulucu::~Ulucu() {
}

static void anyan_interact_callback(CMD_PARAM_STRUCT *args) {
    warnf("anyan cmd: %d\n", args->cmd_id);
    switch (args->cmd_id) {
        case VIDEO_CTRL: break;
        default:
            warnf("undefine cmd [%d]\n", args->cmd_id);
            break; 
    }
}

bool Ulucu::init() {

    Dev_SN_Info oem_info = {0};
    oem_info.OEMID = 100002;
    char *SN = "Ub0000000123456444NN";
    //char* SN = "0000000123456444";
    memcpy(oem_info.MAC, "0A0027000004", sizeof("0A0027000004"));
    memcpy(oem_info.SN, SN, strlen(SN));
    strcpy(oem_info.OEM_name, "Ub");
    strcpy(oem_info.Factory, "ULUCU");
    strcpy(oem_info.Model, "UL-LAPI08");

    //检测网络连接 
    Dev_Attribut_Struct attr = {0};
    attr.block_nums = 200;  // 流缓冲区 200 * 8k
    attr.channel_num = 1;   // 设备通道数
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
    attr.use_type = 0; //设备使用类型 0：对外销售设备，1：测试设备，2：演示设备。 默认0。
    attr.max_rate = UPLOAD_RATE_4;
    attr.max_rate = UPLOAD_RATE_3;

    int ret = Ulu_SDK_Init_All(&oem_info, &attr, NULL);
    Ulu_SDK_Set_Interact_CallBack(anyan_interact_callback);

    return true;
}



}