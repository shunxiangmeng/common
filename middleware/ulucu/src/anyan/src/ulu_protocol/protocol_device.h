
#ifndef __PROTOCOL_DEVICE_H__
#define __PROTOCOL_DEVICE_H__

#include "typedefine.h"
#include "aystream.h"
#include "ayentry.h"
#include "channel.h"
#include "Anyan_Device_SDK.h"

#define DEVICE_OK                           0
#define DEVICE_ERR                          -1
#define DEVICE_ERR_SESSION_CREATE_FAIL		-2
#define DEVICE_ERR_TOKEN_CHECK_FAIL         -3
#define DEVICE_ERR_IS_WILD                  -4 /* device not bind */
#define DEVICE_ERR_WRONG_TOKEN_DEST         -5
#define DEVICE_ERR_TOKEN_DECRYPT_FAIL       -6

#define MAX_TCP_PACKET_SIZE                 64*1024
#define PIECE_SIZE                          16*1024

/***** Device VIP Permisssion , 16: 0x1000 - 0x8000000 ********/
#define C3VP_CLOUD_STORAGE                  0x1000      /* 云存储 */
#define C3VP_MINRATE_STREAM                 0x2000
#define C3VP_MAXRATE_STREAM                 0x4000
#define C3VP_UPLOAD_AUDIO                   0x8000      /* 开启声音上传 */

#define C3VP_UPLOAD_AUTO                    0x10000     /* 开启自动上传 */
#define C3VP_MDCLOUD_STORAGE                0x20000     /* 移动侦测云存储 */
#define C3VP_UPLOAD_ONLY_IFRAME             0x40000     /* 无任何用户观看时只上传I帧,有用户观看才上传所有帧 */
#define C3VP_UNBIND_DEVICE                  0x80000     /* 孤设备 */

#define C3VP_SERVER_G711_TO_AAC	            0x100000
#define C3VP_HIST_TS_PACK_PLAY	            0x200000
#define C3VP_MULTI_POINT_LOGIN	            0x400000
#define C3VP_CLOSE_DEV_ATONCE	            0x800000    /* 没用户观看时,立即关闭 */

#define C3VP_UPLOAD_STREAM	                0x1000000 
#define C3VP_HDCLOUD_STORAGE                0x2000000


//#define C3VP_PERMISSION_13    0x1000000
#define C3VP_PERMISSION_14	                0x2000000
#define C3VP_PERMISSION_15	                0x4000000
#define C3VP_PERMISSION_16	                0x8000000

typedef struct __token_t
{
    uint16 token_bin_length;
    uint8 token_bin[256];
} token_t;

typedef struct __c3_error_t
{
    int32 error_code;
    char  error_description[256];
} c3_error_t;

enum TI_COMPILE_COMAND
{
    gcc_x86_Linux = 0,
    arm_linux = 1,
    arm_none_linux_gnueabi = 2,
    arm_hismall_linux = 3,
    arm_hisiv100nptl_linux = 4,
    arm_v5t_le = 5,
    arm_linux_gnueabi = 6,
    arm_ambarella_linux_uclibcgnueabihf_ = 7,
    arm_fullhan_linux_uclibcgnueabi = 8,
    arm_cortex_linux_gnueabi = 9,
    arm_hisiv200_linux_gnueabi = 10,
    arm_linux_gnueabihf = 11,
    arm_hisiv400_linux = 12,
    arm_hisiv200_linux = 13,
    arm_hisiv300_linux = 14,
    arm_hik_v7a_linux_uclibcgnueabi = 15,
    //-------------------从51200以上都是自己的-----------------
    HAIXIN_HI3518 = 51200,          /* 原来 200 */
    XIONGMAI_HI = 51201,            /* 原来 201 */
    NVR_HI3518 = 51202,             /* 原来 202 */
    U4_HI = 51203,                  /* 原来 203 */
};

/* #define    HSI_TARGET 设备宏有编译时控制定义.不在此处定义 */
#if defined (HSI_TARGET_ARM_HISMALL_LINUX)
#define COMPILE_COMMAND				arm_hismall_linux
#define GCC_VERSION_1				3
#define GCC_VERSION_2				4
#define GCC_VERSION_3				3
#elif defined (HSI_ARM_HISIV100NPTL_LINUX)
#define COMPILE_COMMAND				arm_hisiv100nptl_linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#elif defined (TI_ARM_V5T_LE)
#define COMPILE_COMMAND				arm_v5t_le
#define GCC_VERSION_1				4
#define GCC_VERSION_2				2
#define GCC_VERSION_3				0
#elif defined (TI_ARM_LINUX) 		//DIWEILE_TARGET
#define COMPILE_COMMAND				arm_linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				3
#define GCC_VERSION_3				5
#elif defined (TI_ARM_NONE_LINUX_GNUEABI) //DIWEILE_TARGET
#define COMPILE_COMMAND				arm_none_linux_gnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				6
#define GCC_VERSION_3				1
#elif defined (ARM_LINUX_GNUEABI) 	//
#define COMPILE_COMMAND				arm_linux_gnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				6
#define GCC_VERSION_3				3
#elif defined (ARM_AMBARELLA_LINUX) 	//
#define COMPILE_COMMAND				arm_linux_gnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				6
#define GCC_VERSION_3				3
#elif defined (ARM_FULLHAN_LINUX) 	//
#define COMPILE_COMMAND				arm_fullhan_linux_uclibcgnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				5
#define GCC_VERSION_3				1
#elif defined (ARM_CORTEX_LINUX) 		//
#define COMPILE_COMMAND				arm_cortex_linux_gnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				5
#define GCC_VERSION_3				1
#elif defined (ARM_HISIV200_LINUX) 		//
#define COMPILE_COMMAND				arm_hisiv200_linux_gnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				5
#define GCC_VERSION_3				1
#elif defined (ARM_GNUEABIHF64_LINUX) 		//
#define COMPILE_COMMAND				arm_linux_gnueabihf
#define GCC_VERSION_1				4
#define GCC_VERSION_2				5
#define GCC_VERSION_3				1
#elif defined (HSI_ARM_HISIV400_LINUX)//
#define COMPILE_COMMAND				arm_hisiv400_linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				8
#define GCC_VERSION_3				3
#elif defined (HSI_ARM_HISIV300_LINUX)//
#define COMPILE_COMMAND				arm_hisiv300_linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				8
#define GCC_VERSION_3				3
#elif defined (HSI_ARM_HISIV200_LINUX)//
#define COMPILE_COMMAND				arm_hisiv200_linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#elif defined (TI_ARM_HIK_V7A_LINUX_UCLIBCGNUEABI)//
#define COMPILE_COMMAND				arm_hik_v7a_linux_uclibcgnueabi
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				6
//-------------------------------自己公司的设备--------------------------------------
#elif defined (HAIXIN_U1_HI3518)
#define COMPILE_COMMAND				HAIXIN_HI3518
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#elif defined (XIONGMAI_HI3518)
#define COMPILE_COMMAND				XIONGMAI_HI
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#elif defined (NVR_TYPE_HI3518)
#define COMPILE_COMMAND				NVR_HI3518
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#else
#define COMPILE_COMMAND				gcc_x86_Linux
#define GCC_VERSION_1				4
#define GCC_VERSION_2				4
#define GCC_VERSION_3				1
#endif

/* 0 代表动态库,1代表静态库 */
#if defined (STATIC_FLAG)
#define GCC_TYPE_LIB				1
#else
#define GCC_TYPE_LIB				0
#endif

//#define YEAT						15  /* 相对于2000年 */
//#define MONTH						9
//#define DAYS						10
//#define COUNT						1

#define READ_32(pos)					(*(pos) + (*(pos + 1) << 8)&0xf0 + (*(pos + 2) << 16)&0xf00 + (*(pos + 3) << 24)&0xf000)
#define READ_16(pos)					(*(pos) + (*(pos + 1) << 8)&0xf0 + (*(pos + 2) << 16)&0xf00)

//#define MSG_MASK_1					0x01
//#define MSG_MASK_2					0x02
//#define MSG_MASK_4					0x04
//#define MSG_MASK_8					0x08
//#define MSG_MASK_10					0x10
//#define MSG_MASK_20					0x20
//#define MSG_MASK_40					0x40
//#define MSG_MASK_80					0x80

#ifndef UDP_MTU
#define UDP_MTU (1472)
#endif
#define CalcCommandID( _From, _To, ID ) (_From << 24) | (_To << 16) | ID
enum EndPointType {
    unknown_ep      =	0x00,
    device_client   =   0x01,   /* 设备端 */
    entry_server    =   0x02,   /* 接入服务器 */
    stream_server   =   0x03,   /* 流服务器 */
    user_client     =   0x04,   /* 用户端 */
    system_user     =   0x80,   /* 系统用户 */
};

enum DeviceStatus
{
    ON_LINE = 0x0001,               /* 在线 */
    STREAM_LOGIN = 0x0002,          /* 流服务器登录状态位 */
    MOVE_DETECT_ALARM = 0x0004,     /* 移动侦测 */
    VIDEO_LOST_STATUS = 0x0008,     /* 丢失 */
    SHELTER_ALARM = 0x0010,         /* 遮挡 */
    ON_PRIVATE_MODE = 0x0020,       /* 设备处于隐私模式 */
};

#define  STATUS_SET(MASK) 	{sdk_get_handle(0)->devinfo.dev_status_mask |= MASK;}
#define  STATUS_CLR(MASK) 	{sdk_get_handle(0)->devinfo.dev_status_mask &= ~(MASK);}

#define  STATUS_SET_EXT(VAL, MASK) 	{VAL |= MASK;}
#define  STATUS_CLR_EXT(VAL, MASK) 	{VAL &= ~(MASK);}

#define SERIALIZE_DWORD(val, pbyte) {*pbyte++ = (char)val; *pbyte++ = (char)(val >> 8); *pbyte++ = (char)(val >> 16); *pbyte++ = (char)(val >> 24);}
#define SERIALIZE_WORD(val, pbyte) {*pbyte++ = (char)val; *pbyte++ = (char)(val >> 8);}

enum Device_CommandID
{
    CID_DeviceRegisterRequest   = CalcCommandID(device_client, entry_server, 0x01),
    CID_DeviceRegisterResponse  = CalcCommandID(entry_server, device_client, 0x02),

    CID_DeviceLoginRequest      = CalcCommandID(device_client, stream_server, 0x01),
    CID_DeviceLoginResponse     = CalcCommandID(stream_server, device_client, 0x02),

    CID_DeviceActionNofity      = CalcCommandID(stream_server, device_client, 0x03),

    CID_StatusReport            = CalcCommandID(device_client, stream_server, 0x04),
    CID_StatusReportRes         = CalcCommandID(stream_server, device_client, 0x05),

    CID_StreamFrameInfoNofity   = CalcCommandID(device_client, stream_server, 0x06),
    CID_StreamFrameDataRequest  = CalcCommandID(stream_server, device_client, 0x07),
    CID_StreamFrameDataResponse = CalcCommandID(device_client, stream_server, 0x08),

    CID_StreamTSInfoNofity      = CalcCommandID(device_client, stream_server, 0x16),
    CID_StreamTSDataRequest     = CalcCommandID(stream_server, device_client, 0x17),
    CID_StreamTSDataResponse    = CalcCommandID(device_client, stream_server, 0x18),


    CID_ExchangeKeyRequest      = CalcCommandID(unknown_ep, unknown_ep, 0x01),  /* 客户端向登录服务器请求交换密钥 */
    CID_ExchangeKeyResponse     = CalcCommandID(unknown_ep, unknown_ep, 0x01),  /* 登录服务器回复客户端交换密钥请求 */


    CID_S2DTSDataQuery          = CalcCommandID(stream_server, device_client, 0x15),
    CID_D2STSDataInfoNotify     = CalcCommandID(device_client, stream_server, 0x16),
    CID_S2DTSDataRequest        = CalcCommandID(stream_server, device_client, 0x17),
    CID_D2STSDataResponse       = CalcCommandID(device_client, stream_server, 0x18),

    /* 设备查询 */
    CID_DeviceStatusQuery       = CalcCommandID(system_user, device_client, 0x01),
    CID_DeviceStatusQueryResponse = CalcCommandID(device_client, system_user, 0x02),
    /* 对讲数据 */
    CID_S2DUserDataNotify       = CalcCommandID(stream_server, device_client, 0x19),
    CID_D2SUserDataRequest      = CalcCommandID(device_client, stream_server, 0x20),

    /* 厂商自定义数据 */
    CID_S2D_OEMData_Transfer    = CalcCommandID(stream_server, device_client, 0x21),
    CID_D2S_OEMData_Transfer    = CalcCommandID(device_client, stream_server, 0x22),
    /* 截图 */
    CID_D2SScreenShotNotify     = CalcCommandID(device_client, stream_server, 0x30),

    CID_S2DNVRHistoryListQuery  = CalcCommandID(stream_server, device_client, 0x31),
    CID_D2SNVRHistoryListNotify = CalcCommandID(device_client, stream_server, 0x32),

    /**** Client <------> Device *****/
    CID_C2DLoginRequest =   CalcCommandID(user_client, device_client, 0x01),
    CID_D2CLoginResponse=   CalcCommandID(device_client,user_client,0x02),

    CID_D2CStatusReport	=   CalcCommandID(device_client,user_client,0x03),
    CID_C2DStatusReportRes  =	CalcCommandID(user_client,device_client,0x04),

    CID_C2DActionNotify =   CalcCommandID(user_client, device_client, 0x05),
    CID_D2CStreamFrameDataResponse =   CalcCommandID(device_client, user_client, 0x06),

	//--------------------------------------------------------------------------------
	//设备能力上报接口
	 CID_DeviceAbilityReport= CalcCommandID(device_client,stream_server,0x09),
};

#pragma pack(1)

typedef struct
{
    uint16  size;       /* 消息长度 */
    uint32  msg_type;   /* 消息类型 */
}
MSG_HEADER_C3;

typedef struct __device_id_t
{
    uint8  device_id_length;
    uint8  device_id[21];
}
device_id_t;

typedef struct
{
    uint32  IP;
    uint16  Port;
}
addr_Info_section;

/* 设备注册请求消息体 */
typedef struct
{
    uint32      mask;               /* 掩码控制发送的消息 */
    uint32      flag;

    /* 0x0001 设备登录 */
    device_id_t  did;               /* 设备id */
    uint32       status_mask;       /* 设备各种状态 */
    //0X0002
    uint32      stream_serv_ip;     /* 流服务器ip和端口 */
    uint16      stream_serv_port;
    token_t     token;              /* 返回的 */
    //0x0004
    uint16      vs[4];		 		/* 版本*/
    uint8       dev_type; 			/* 设备类型 1 dvr, 2 nvr, 3 ipc, 4 box */
    uint8       video_type;
    uint8       channel_num;        /* 通道数 */
    uint16      min_rate;
    uint16      max_rate;
    //0x0008
    addr_Info_section    hi_private_addr;

    //0x0010
    token_t     reg_token;	        /* 提交上去的 */
    //0x0020
    uint32      entry_ip;           /* 发送目标ip */
    //0X0040
    uint16      video_width;        /* 图像宽高 */
    uint16      video_height;
	//0X80
    uint8       channel_mask_len;   /* 通道掩码长度,最长16个字节 */
    uint8       channel_mask[16];   /* 通道在线掩码:offline  1:online，最多128个通道，如果通道需要扩展，再扩展字段 */

    //0x100
    char rtsp_url[128];
} MsgDeviceRegisterRequest;

//CDataStream& operator<<( CDataStream& ds, MsgStatusReportRes & msg );
//CDataStream& operator>>( CDataStream& ds, MsgStatusReportRes& msg );
//---------------------------------MsgDeviceRegisterResponse-----------------------------------
/* 设备注册响应消息体 */
typedef struct
{
    uint32      mask;
    uint32      flag;

    //0x01
    device_id_t did;
    c3_error_t  ret;    /* 0:正常 -1:失败 */
    uint16      expected_cycle;//s
    //0x02
    uint32      IP;     /* 公网ip */
    uint16      Port;
    //0x04
    uint32      stream_serv_ip;
    uint16      stream_serv_port;
    token_t     token;

    //0x08
    uint8       entry_num;
    addr_Info_section       paddr[5];

    //0X10
    uint32      current_time;
    //0x20
    uint16      update_vs[4];       /* 更新版本 */
    uint32      update_package_crc32;
    char        update_addr[256];   /* 更新地址 */
}
MsgDeviceRegisterResponse;

//-------------------------------------------DeviceActionNofity----------------------------------------------
typedef enum
{
    enm_pp_noop = 0,
    enm_pp_set,
    enm_pp_clear,
    enm_pp_jump
}preset_position_action_t;

typedef struct
{
    uint32  mask;
    uint32  flag;

    //0x01
    uint8   enable_upload;      /* 通知设备上传数据（为1时），或者停止上传数据（为0时）*/
    uint16  upload_rate;        /* 上传码率，为0表示上传或者停止上传所有码率 */
    uint8   channel_index;      /* 通道索引，为0表示上传或者停止上传所有通道数据 */

    //0X02
    uint32  action;             /* see variable 'device_control' defined in 'protocol_header.h' */
    uint8   action_channel;     /* 想要控制的通道索引 */

    /* 0X04 对讲功能 */
    char    *from_user;
    uint32  seq;
    uint32  audio_total_len;
    uint32  audio_offset;
    uint32  audio_len;
    uint8   *audio_data;         //8*1024
    //0x08
    uint16  expected_screenshot_cycle;  /* 期望的截图周期 */

    //0x10
    uint32  ptz_steps;          /* PTZ步长，防止stop指令的丢失导致设备无限旋转 */

    //0x20
    uint8   sdcard_deal;        // 0  format sdcard , 1 query sd card status

    //0x40
    uint8   ctrl_record;        // 0 set recode mode;1 stop  ;2 start
    uint8   set_record_mode;    // @AYE_TYPE_RECORD_MODE

    //0x80   /* 预置位操作 */
    uint8  preset_act;
    uint8  channel_index_for_preset;
    uint16 position_nu;
} MsgDeviceActionNofity;

//-----------------------------------------MsgDeviceLoginRequest----------------------------------------------

typedef struct  /* 登录消息 */
{
    uint32      mask;
    uint32      flag;

    /*
       m_new_timestamp_format_flag = 0x01,
       enm_support_aac_decode = 0x02,//设备默认支持AAC解码,设备不支持时该位置位，由服务器进行aac to g711的编码
       enm_support_screenshot = 0x04,//支持截图
       enm_has_tfcard = 0x08,//该IPC设备有TF卡
    */

    // 0x01
    device_id_t did;                /* 设备id */
    token_t     token;
    uint16      vs[4];              /* 版本 */
    uint32      status_mask;        /* 设备各种状态 */

    uint8       dev_type;           /* 设备类型 1 dvr, 2 nvr, 3 ipc, 4 box */
    uint8       video_type;
    uint8       channel_num;	    /* 通道数 */
    uint16      min_rate;
    uint16      max_rate;

    /* 0X02 本地ip */
    addr_Info_section   hi_private_addr;
    //0x04
    uint32      current_time;
    uint32      current_tick;

    //0x08
    uint8       frame_rate;
    //0X10
    uint16      uchannelcount;
    uint32      uSamplerate;
    uint16      ubitLength;
    //0X20
    uint8       channel_mask_len;   /* 通道掩码长度,最长16个字节 */
    uint8       channel_mask[16];   /* 通道在线掩码:offline  1:online，最多128个通道，如果通道需要扩展，再扩展字段 */
} MsgDeviceLoginRequest;

enum
{
    enm_control_record = 0x01,          /* 控制的是历史视频数据的关闭或打开 */
    enm_enable_audio = 0x02,            /* 启动/关闭设备上的音频上传 */
    enm_enable_video = 0x04,            /* 启动/关闭设备上的视频上传 */
    enm_enable_auto_upload = 0x08,      /* 无需服务器去向设备取数据，设备主动推数据上来 */
    enm_enable_only_i_upload = 0x10,    /* 启动仅I帧上传 */
    enm_device_wild = 0x20,             /* 孤设备 */
    enm_control_record_extend = 0x40,   /* 控制的是历史视频数据的关闭或打开 */
};

typedef struct
{
    uint32          mask;
    uint32          flag;

    //0x01
    device_id_t     did;
    c3_error_t      ret;
    uint32          ip;         /* 服务器端所看到的IP */

    //0X02
    uint8   enable_upload;		/* 通知设备上传数据（为1时），或者停止上传数据（为0时）*/
    uint16  upload_rate;		/* 上传码率，为0表示上传或者停止上传所有码率 */
    uint8   channel_index;		/* 通道索引，为0表示上传或者停止上传所有通道数据 */
} MsgDeviceLoginResponse;

//-------------------------------------------MsgStatusReport----------------------------------------------
typedef struct{
    uint16 channel_index;
    uint32 channel_status;
} Channel_Status;

typedef struct
{
    uint32 mask;
    uint32 flag;

    //0x01
    uint32 status_mask;     /* 设备各种状态 */

    //0x02
    uint8 channel_index;
    uint16 min_rate;        /* 码率发生了改变 */
    uint16 max_rate;

    //0x04
    uint32 current_time;
    uint32 current_tick;

    //0X08
    uint32 stream_serv_ip;
    uint16 stream_serv_port;
    token_t new_token;
    //0X10
    uint8 channel_mask_len;     /* 通道掩码长度, 最长16个字节 */
    uint8 channel_mask[16];     /* 通道在线掩码,最大128个通道,如果通道需要扩展,在扩展字段 */
    /* 0x20 透传 */
    uint16  oem_data_len;
    uint8   *oem_data;

    //0X40
    uint16 channel_status_num;
    Channel_Status channels_status[MAX_CHANNEL_NUM];

    //0x80
    uint32  channel_q_num;      /* 帧队列缓存数量 */
    uint32  report_rslt;        /* 帧上报结果 */
    uint32  callback_status;    /* 回调线程活动标识 */

    //0x100
    uint8   channel_index_zoom;
    uint32  zoom_value;         /* 扩大10倍的值 */

    //0x200
    uint8   sdcard_status;      // @AYE_TYPE_SD_STATUS
    uint8   record_mode;        // @AYE_TYPE_RECORD_MODE
} MsgStatusReport;

typedef struct MsgStatusReportRes
{
    uint32 mask;
    uint32 flag;
    //0x01
    uint16 expected_cycle;
    //0x02
    c3_error_t ret;
    //0x04
    uint16  oem_data_len;
    uint8   *oem_data;
} MsgStatusReportRes;


enum device_control
{
    enm_device_up = 1,
    enm_device_down = 2,
    enm_device_left = 3,
    enm_device_right = 4,
    enm_device_zoom_in = 5,
    enm_device_zoom_out = 6,
    enm_device_stop = 7,
    enm_server_transfer_audio = 8,
    enm_server_stop_transfer_audio = 9,
    enm_server_reset_device = 10,
    enm_server_reboot_device = 11,
};


enum
{
    enm_audio_flag = 0x01,
    enm_i_frame_flag = 0x02,
    enm_fh8610_frame_tag = 0x04,
    enm_fh8610_frame_last_block_tga = 0x08,
    enm_record_frame_flag = 0x10,           /* 历史帧 */
    enm_move_detect_frame_flag = 0x20,      /* 移动侦测帧 */
    enm_human_detect_frame_flag = 0x40,     /* 人形检测 */
};

typedef struct
{
    uint32 	mask;
    uint32 	flag;

    //0x01
    uint8   channel_index;
    uint16  update_rate;
    uint32  frm_seq; 		/* 帧序号 */
    uint32  frm_ts;
    uint32  frm_size; 		//
    //0x02
    uint32  crc32_hash;		/* CRC校验 */
    //0x04
    uint32  datalen;
    char    *pdata;
    //0x08
    uint32  av_frm_seq;     /* 音频帧序号 */
} MsgStreamFrameInfoNofity; /* 帧通知 设备->服务器 */

enum {
    enm_type_periodical = 0x0,
    enm_type_instant = 0x01,
};

typedef struct
{
    uint32  mask;
    uint32  flag;

    //0x01
    uint16  pic_MTU;        /* 图片分包大小 */
    uint8   channel_index;
    uint16  update_rate;
    uint32  status;
    uint32  screenshot_ts;  /* 时间戳 */
    uint32  screenshot_size;
    //0x02
    uint32 	offset;
    uint32  length;
    uint8   *data;
    //0X04
    uint32 	crc32_hash;
    //0x08
    uint32 	img_type_len;
    char 	*img_type;      //[128];
}MsgD2SScreenShotNotify;

typedef struct
{
    uint32  mask;
    uint32  flag;
    //0x01
    uint8   channel_index;
    uint16  update_rate;
    uint32  frm_seq; 	        /* 帧序号 */
    uint32  offset;
    uint32  length;
} MsgStreamFrameDataRequest;    /* 服务器请求帧数据 */

typedef struct
{
    uint32 mask;
    uint32 flag;

    //0x01
    uint8  channel_index;
    uint16 update_rate;

    uint32 frm_seq;             /* 帧序号 */
    uint32 offset;
    uint32 length;

    //0X02
    char    *data;

    //0x04
    uint32  frm_ts;             /* 在设备端主动推数据的模式下，带上如下两个数据 */
    uint32  frm_size;
    //0x08
    uint32  av_frm_seq;
    //0X10
    uint32  frm_time;           /* 秒时间戳 */
    //0x20
    uint8   device_hash[2];     /* 设备的HASH值 */
    //0x40
    uint32 alarm_type;          // 1:human detect
    uint32 alarm_time;          //utc time
    //0x80
    uint8 media_video_type;     //Media_VideoCodecID
    uint8 media_audio_type;     //Media_AudioCodecID
    uint8 reserved;
} MsgStreamFrameDataResponse;

enum EnumVideoCodecID
{
    Enum_Video_H264 = 0x01,
    Enum_Video_H265 = 0x02
};

enum EnumAudioCodecID
{
    Enum_Audio_AAC   = 0x01,
    Enum_Audio_G711A = 0x02,
    Enum_Audio_G711U = 0x03,
    Enum_Audio_MP3   = 0x04
};

enum
{
	enm_do_ts_on_device = 0x00,
	enm_do_ts_on_server = 0x01,
};

typedef struct
{
    uint32  mask;
    uint32  flag;
    //0x01
    uint8   channel_index;
    uint16  rate;
    uint32  ts_ts;
    uint32  ts_duration;
    //0x02
    uint32  req_id_length;
    char    *request_id;
} MsgS2DTSDataQuery;

enum
{
    enm_has_ts = 0x01,
};

typedef struct
{
    uint32      mask;
    uint32      flag;

    //0x01
    uint8       channel_index;
    uint16      rate;
    uint32      ts_ts;
    //0X02
    uint32      ts_size;
    uint32      ts_duration;
    //0x04
    uint32      length;
    uint8       *ts_data;
    //0x08
    uint32      req_id_length;
    char        *request_id;
} MsgD2STSDataInfoNotify;

typedef struct
{
    uint32  mask;
    uint32  flag;
    //0x01
    //device_id_t did;
    uint8 channel_index;
    uint16 rate;
    uint32 ts_ts;
    uint32 offset;
    uint32 length;
} MsgS2DTSDataRequest;

typedef struct
{
    uint32  mask;
    uint32  flag;
    //0x01
    uint8   channel_index;
    uint16  rate;
    uint32  ts_ts;
    uint32  offset;
    uint32  length;
    //0X02
    uint8   *ts_data;
    //0x04
    uint32  ts_size;        /* 整体文件大小 */
    uint32  ts_duration;    /* 整体时间长度 */
    //0x08
    uint32  req_id_length;  /* id长度 */
    char    *request_id;    /* id内容 512 */
} MsgD2STSDataResponse;     /* 设备到服务器 */


//------------------------------------------------对讲数据消息结构---------------------------
typedef struct
{
    uint32	mask;
    uint32  flag;
    //0x01
    char 	*user_name;
    uint32 	seq;
    uint32 	audio_total_len;
} MsgS2DUserDataNotify;  /* 对讲----服务器->设备 */

typedef	struct
{
    uint32  mask;
    uint32  flag;

    //0x01
    char    *user_name;
    uint32  seq;
    uint32  audio_offset;
    uint32  audio_len;
} MsgD2SUserDataRequest; /* 对讲----设备->服务器 */

//typedef struct
//{
//	uint32 start_time;
//	uint32 end_time;
//}NVR_history_block;
typedef struct
{
    uint32  mask;
    uint32  flag;
    device_id_t did;
    uint8 channel_index;
    uint16 rate;        //0X02
    uint32 start_time;  /* 开始时间 */
    uint32 end_time;    /* 结束时间 */
} MsgS2DNVRHistoryListQuery;

typedef struct
{
    uint32  mask;
    uint32  flag;
    //0x01
    device_id_t did;
    uint8 channel_index;
    uint16 rate;
    //0x02
    uint32 start_time;      /* 开始结束时间对，用来标记对应的MsgS2DNVRHistoryListQuery */
    uint32 end_time;
    //0x04
    uint32 list_len;
    NVR_history_block hList[128];
    uint32 seq;             /* 若hList不足以容纳请求的时间跨度end_time - start_time，需要多个MsgC2SNVRHistoryListResponse消息 */
} MsgD2SNVRHistoryListNotify;

typedef struct
{
    uint32  mask;
    uint32  flag;

    //0x01
    device_id_t did;
} MsgC2DLoginRequest;

typedef struct
{
    uint32  mask;
    uint32  flag;// 0x01 - auto_push,0x02 - offline,0x04 - has tfcard

    // 0x01
    device_id_t did;
    c3_error_t ret;

    // 0x02
    uint8 media_type;
    uint8 dev_type;

    // 0x04
    uint16 uchannelcount;
    uint32 uSamplerate;
    uint16 ubitLength;

    // 0x08
    uint32 status_mask;
} MsgD2CLoginResponse;

#if 0
typedef struct
{
    uint32  mask;
    uint32  flag;
}MsgC2DStatusReportRes;
typedef struct
{
    uint32  mask;
    uint32  flag;
}MsgD2CStatusReport;
typedef struct
{
    uint32 mask;
    uint32 flag;
}MsgC2DActionNotify;
#else
typedef MsgStatusReport MsgD2CStatusReport;
typedef MsgStatusReportRes MsgC2DStatusReportRes;
typedef MsgDeviceActionNofity MsgC2DActionNotify;
#endif

#pragma pack()

typedef struct
{
    uint32      dev_status_mask;    /* 设备状态掩码 */
    uint32      local_net_ip;       /* 本地ip和port */
    uint16      entry_timeout;
    uint16      max_rate;           /* 最大码率 */
    uint16      min_rate;           /* 最小码率 */

    uint16      video_width;        /* 图像宽 */
    uint16      video_height;       /* 图像高 */

    char        audio_type;         /* 音频类型 0:acc, 1:g711_a, 2:g711_u, 3:mp3 */
    char        video_type;

    char        audio_chnl; 		/* 音频通道,单通道 1, 双通道 2 */
    uint16      audio_smaple_rt;	/* 音频采样率 */
    uint16      audio_bit_width;	/* 位宽 */

    uint32      block_nums;
    uint8       channelnum;         /* 通道数量 */
    char        dev_type;           /* 设备类型0 未知 1 dvr, 2 nvr, 3 ipc, 4 box */
    char        ptz_flag;           /* 是否支持ptz控制 0 不知支持, 1 只支持支持上下左右, 2 支持上下左右和变焦 */
    char        mic_flag;           /* 是否有拾音器 0 没有, 1 有 */
    char        can_rec_voice;      /* 可以接受音频 0 不支持, 1 支持 */
    char        upload_ctrl;        /* 上传控制标志 0 不上传,1 上传,默认不上传 */
    char        hard_disk;          /* 是否有硬盘  0 没有, 1 有 */
    char        rw_path[200];       /* 设备可读写路径 */
    Dev_SN_Info Sn_info;            /* 鉴定设备的一些信息 */
    device_id_t did;                /* 设备id */
    char        device_id_str[21];  /* 设备id 的字符串 */
    char        wifi_online_broad;
    char        has_tfcard;         /* 是否有TF卡 0: 没有 1:有 */

    uint32      resp_json_code;
    uint32      vip_flag;           /* 设备服务权限标志 */
    char        strm_conn_stat;     /* 0 - ok, 1 - fail */
    uint32      stream_serv_ip;
    uint16      stream_serv_port;
    token_t     token;              /* stream  server token */
    char        token_update_flag;

    uint32      public_net_ip;
    uint16      public_net_port;

    uint16      expected_cycle;
    uint16      expt_cycle_strm;

    token_t     entry_token_b64;    //base64 decode

    uint8       use_type;		    /* 设备使用类型：0：对外销售设备，1：测试设备，2：演示设备。 默认0 */
    uint8       devstatus_update_flag;
    uint8       channel_mask[16];	/* one bit one channel, 0 - offline  1 - online */
    Channel_Status  chnl_status[MAX_CHANNEL_NUM];

    int         sync_time_flag;

    char        rtsp_url[128];
    int         sdcard_status;
    int         record_mode;
    char        entry_token_base64[512];
    char        entry_ver[64];

    char        now_ack_tracker_addr[32];
	uint8       max_stream_num_per_chn; /* 每个通道最多同时允许发送的流个数 */
} Dev_info_struct;


int Pack_MsgDeviceRegisterRequest(char *buf, MsgDeviceRegisterRequest *msg);
int Pack_MsgDeviceLoginRequest(char *buf, MsgDeviceLoginRequest *msg);
int Pack_MsgStreamFrameInfoNofity(char *buf, MsgStreamFrameInfoNofity *msg);
int Pack_MsgStreamFrameDataResponse(char *buf, MsgStreamFrameDataResponse *msg);
int Pack_MsgStatusReport(char *buf, MsgStatusReport *msg);
int Pack_MsgD2STSDataInfoNotify(char *buf, MsgD2STSDataInfoNotify *msg);
int Pack_MsgD2STSDataResponse(char *buf, MsgD2STSDataResponse *msg);
int Pack_MsgD2SUserDataRequest(char *buf, MsgD2SUserDataRequest *msg);
int Pack_MsgD2SScreenShotNotify(char *buf, MsgD2SScreenShotNotify *msg);
int Pack_MsgD2SNVRHistoryListNotify(char *buf, MsgD2SNVRHistoryListNotify *msg);

int Unpack_Msg(int skfd, char *pbuf, int len);
uint8 *Check_ID_Error(uint8 *buf, int len, uint32 *err);

int Pack_MsgD2CStatusReport(char *buf, MsgStatusReport *msg);
int Pack_MsgD2CLoginResponse(char *buf,MsgD2CLoginResponse *msg);
int Pack_MsgD2CStreamFrameDataResponse(char *buf, MsgStreamFrameDataResponse *msg);

//-----------------------------------------MsgDeviceAbilityReport--------------------------------------------------------------------
// device ability report to server
//add by che
typedef struct _MsgDeviceAbilityReport
{
    uint32  mask;
    uint32  flag;

    //0x01
    uint8 max_stream_num_per_chn;
} MsgDeviceAbilityReport;

int Pack_MsgDeviceAbilityReport(char *buf, MsgDeviceAbilityReport *msg);

//------------------------------------------------------------------------------------------------------
#endif
