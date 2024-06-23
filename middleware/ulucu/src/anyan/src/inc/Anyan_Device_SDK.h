/********************************************************************************************
  版权所有 (C), 2001-2024, 上海悠络客电子科技有限公司
 *********************************************************************************************/
#ifndef __ANYAN_DEVICE_SDK_H__
#define __ANYAN_DEVICE_SDK_H__

#ifdef WIN32
#define AYAPI_ATTR
#else
#define AYAPI_ATTR __attribute__((deprecated))
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef char            int8;
typedef short           int16;
typedef int             int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef unsigned long long  uint64;

#define	 AY_MAX_CHANNEL_NUM  128

/* 实时流帧类型定义 live stream frame type macro */
#define  CH_I_FRM           0
#define  CH_P_FRM           1
#define  CH_AUDIO_FRM       2

/* 回放流帧类型定义 history stream frame type macro */
#define  CH_HIS_I_FRM       3
#define  CH_HIS_P_FRM       4
#define  CH_HIS_AUDIO_FRM   5

/* 上传流使用的码率等级,没有对应码率可以找最接近的一个 */
#define  UPLOAD_RATE_1       384
#define  UPLOAD_RATE_2       500
#define  UPLOAD_RATE_3       700
#define  UPLOAD_RATE_4       1000

typedef struct
{
    int     block_nums;                     /* 开辟的缓冲区的数量单位是8k*block_nums */
    uint8   channel_num;                    /* 通道数量 */
    uint16  max_rate;                       /* 最大码率 */
    uint16  min_rate;                       /* 最小码率 */
    char    ptz_ctrl;                       /* 是否支持ptz控制 0 不支持,1 支持上下左右 2 支持上下左右和变焦,3 支持上下左右和预置位, 4 支持上下左右、变焦和预置位 */
    char    dev_type;                       /* 设备类型0 未知 1 dvr, 2 nvr, 3 ipc, 4 box */

    char    mic_flag;                       /* 是否有拾音器 0 没有, 1 有 */
    char    can_rec_voice;                  /* 可以接受音频 0 不支持, 1 支持 */
    char    hard_disk;                      /* 是否有本地存储 0 没有, 1 有硬盘 */
    char    *p_rw_path;                     /* 可读写路径,存放sn,库文件等 */

    char    audio_type;                     /* 音频类型 0:aac, 1:g711_a, 2:g711_u, 3:mp3, 4:adpcm,5:G726 */
    char    audio_chnl;                     /* 音频通道,单通道 1, 双通道 2 */
    uint16  audio_smaple_rt;                /* 音频采样率 */
    uint16  audio_bit_width;                /* 位宽 */

    uint8   use_type;                       /* 设备使用类型：0：对外销售设备，1：测试设备，2：演示设备。 默认0 */
    uint8   has_tfcard;                     /* 是否有TF卡 0: 没有 1:有 */
    uint8   video_type;                     /* 视频类型 0: H264, 1: HEVC(H265) */
    uint8   lan_disable;                    /* 禁用局域网功能 0: 启用(default), 1:禁用(disable) */
    uint8   Reserved[16];                   /* 预留字段, Reserved[0]: max_stream_num_per_chn */
} Dev_Attribut_Struct;

typedef struct
{
    int     channelnum;                     /* 通道号 */
    uint16  bit_rate;                       /* 码率 */
    uint16  frm_type;                       /* 帧类型 CH_I_FRM, CH_P_FRM, CH_AUDIO_FRM */

    uint32  frm_id;                         /* 帧号流水号所有的 2015/3/11 后此字段无效.无需处理或者填充 */
    uint32  frm_av_id;                      /* 帧号总流水号i 帧或者P帧或者音频帧. 2015/3/11 后此字段无效.无需处理或者填充 */

    uint32  frm_ts;                         /* 时间戳 ms */
    char    *pdata;                         /* 数据 */
    uint32  frm_size;                       /* 帧大小 */
    uint16  media_video_type;               /* 视频编码格式 @Media_VideoCodecID */
    uint16  media_audio_type;               /* 音频编码格式 @Media_AudioCodecID */
} Stream_Event_Struct;

typedef struct
{
    int     channel_index;                  /* 通道号 */
    uint16  update_rate;                    /* 码率 */
    uint16  alarm_type;                     /* 报警类型 参见:ALARM_TYPE */
    uint8   picture_type;                   /* 图片格式,暂未使用,只支持jpg */
    uint32  screenshot_ts;                  /* 时间戳 */
    uint32  pic_length;                     /* 截图长度 */
    uint8   *pic_data;                      /* 截图数据 */
} Screen_Shot_Struct;

typedef struct
{
    int     channelnum;                     /* 通道号,start 1 */
    uint16  bit_rate;                       /* 码率 */
    uint32  ts_ts;                          /* 帧类型 起始时间戳 */
    uint32  offset;                         /* 偏移 */
    uint32  length;
    uint8   *ts_data;
    uint32  ts_size;                        /* 整体文件大小 */
    uint32  ts_duration;                    /* 整体时间长度 */
    uint16  media_video_type;               /* 视频编码格式 */
    uint16  media_audio_type;               /* 音频编码格式 */
} Stream_History_Struct;

typedef struct
{
    uint16  channelnum;                     /* 通道号 */
    uint16  bit_rate;                       /* 码率 */
    uint16  frm_type;                       /* 帧类型 I P */
    uint32  frm_ts;                         /* 时间戳 */
    uint8   *pdata;                         /* 帧数据 */
    uint32  len;                            /* 帧数据长度 */
    uint16  media_video_type;               /* 视频编码格式 @Media_VideoCodecID */
    uint16  media_audio_type;               /* 音频编码格式 @Media_AudioCodecID */
} Stream_History_Frm_Struct;

typedef struct
{
    uint32 start_time;
    uint32 end_time;
} NVR_history_block;

typedef struct
{
    uint8 channel_index;
    uint16 rate;
    uint32 start_time;      /* 开始时间 */
    uint32 end_time;        /* 结束时间 */
    uint32 list_len;
    NVR_history_block hList[128];
    uint32 seq;
} HistoryListQuery_Struct;

typedef enum
{
    PTZ_CTRL_LEFT = 0x01,
    PTZ_CTRL_RIGHT,
    PTZ_CTRL_UP,
    PTZ_CTRL_DOWN,
    PTZ_CTRL_ZOOM_IN,
    PTZ_CTRL_ZOOM_OUT,
    PTZ_CTRL_PTZ_STOP,
    PTZ_CTRL_OPEN_VIDEO,
    PTZ_CTRL_CLOSE_VIDEO,
    PTZ_CTRL_SYS_RESET,
    PTZ_CTRL_SYS_REBOOT,
    PTZ_CTRL_MIRROR,
    PTZ_CTRL_FLIP,
} PTZ_CTRL_ENUM;

typedef  enum
{
    /* 视频控制
     * args: [0] - enable flag, [1]~[2] - rate
     */
    VIDEO_CTRL,

    /* 历史回放
     * args: [0]~[1] - rate, [2]~[5] - start time(UTC time), [6]~[9] - duration seconds,
     * [10] - ULK_HIST_VIDEO or ULK_HIST_NORMAL,see @ULK_HISTORY_VIDEO_ENUM datatype
     */
    HISTORY_CTRL,

    /* 音频控制
    * args: [0] - enable flag, [1]~[2] - rate
    */
    AUDIO_CTRL,

    /* 对话控制: Not Finished */
    TALKING_CTRL,

    /* 抓拍控制: Not Finished */
    CAPTURE_PICTURE_CTRL,

    /* 云台控制
    * args: [0] - stop(0)/start(1), [1] - see @PTZ_CTRL_ENUM define, [2]~[5] - ptz steps
    */
    PTZ_CTRL,

    /* 设置预置点
    * args: [0]~[1] - preset position
    */
    PTZ_SET_PRE_LOCAL,

    /* 调用预置点
    * args: [0]~[1] - preset position
    */
    PTZ_CALL_PRE_LOCAL,

    /* 设置看守位: Not Finished */
    PTZ_SET_WATCH_LOCAL,

    /* 设置巡航: Not Finished */
    PTZ_SET_CRUISE,

    /* 手动,看守位,巡航: Not Finished */
    PTZ_SET_MODE,

    /* 报警控制 alarm control
    * args: AY_Alarm_Struct data(memcpy)
    * args_len = sizeof(AY_Alarm_Struct)
    */
    ALARM_CTRL,

    /* 获取设备参数: Not Finished */
    DEVICE_PARAM_GET,

    /* 设置设备参数: Not Finished */
    DEVICE_PARAM_SET,

    /* 校时 NTP Time sync
    * args: [0]~[3] - UTC Timestamp to reference,need to adjust time by timezone.
    */
    TIME_SYN,

    /* 错误信息
    * args: [0]~[N] - info string
    */
    ERROR_INFO,

    /* 透传数据
    * args: [0]~[N] - user private data
    */
    USER_DATA,

    /* WIFI密码: Not Finished */
    WIFI_PASSWORD,

    /* 停止回放
    * args: None
    */
    HISTORY_STOP,

    /* 回放查询
    * args: [0]~[1] - rate, [2]~[5] - start time(UTC time), [6]~[9] - end time
    */
    HISTORY_LIST,

    /* SD卡控制
    * args: [0] - action: 0 format sd card,1 query sd status
    */
    SD_CARD_CTRL,

    /* 录像控制
    * args: [0] - action: 0 set record mode, 1 stop record, 2 start record; [1] - record mode value(when set)
    */
    RECORD_CTRL,

    /* 清除预置位
    * args: [0]~[1] - preset position
    */
    PTZ_CLEAR_PRESET,

    /* 查询移动侦测敏感度 query motion detect sensitivity
    * args: None
    */
    EXT_MDSENSE_QUERY,

    /* 设置移动侦测敏感度 set motion detect sensitivity
    * args: [0] - switch flag 0 or 1, [1] - sensitivity level value 10 - 30(low/medium/high)
    */
    EXT_MDSENSE_SET,

    /* 查询声音侦测敏感度 query audio detect sensivity
    * args: None
    */
    EXT_ADSENSE_QUERY,

    /* 设置声音侦测敏感度 set audio detect sensivity
    * args: [0] - switch flag 0 or 1, [1] - sensitivity level value 10 - 30(low/medium/high)
    */
    EXT_ADSENSE_SET,

    /* 查询侦测记录 query alarm detect record
    * args: [0]~[3] - start time(UTC time), [4]~[7] - end time
    */
    EXT_DETECT_RECORD_QUERY,

    /* 查询sd卡状态 query sd card status information
    * args: None
    */
    EXT_SDCARD_QUERY,

    /* 设置sd控制 control sd card
    * args: [0]~[1] - action cmd 1000: format sdcard, 1001: set record flag
    *	 [2] - when cmd is 1001, record flag 0:disable 1:enable normal record 2: enable alarm record
    */
    EXT_SDCARD_CONTROL,

    /* 查询设备预置位 query device preset position
    * args: None
    */
    EXT_PRESET_QUERY,

    /* 设置预置位控制 set device preset position
    * args:[0] - action subcmd,1:set 2: delete 3:execute, [1] - preset index
    * [2]~[5] - horizontal coordinates,[6]~[9] - vertical coordinates, [10]~[11] - lens zoom multiples
    */
    EXT_PRESET_SET,

    /* 查询视频翻转 query device video flip/mirror parameters
    * args: None.
    */
    EXT_IMAGE_QUERY_TURN,

    /* 设置视频翻转 set device image flip/mirror
    * args: [0] - horizontal turn(mirror) cmd 0:close 1:turn 180 degree, [1] - vertical turn(flip) cmd,0:close 1:turn 180 degree
    */
    EXT_IMAGE_SET_TURN,

    /* 重启设备 reboot device
    * args:[0] - subcmd, 1:reboot
    */
    EXT_REBOOT_DEVICE,

    /* 恢复出厂设置 restore factory settings
    * args：[0] - subcmd, 1: reset
    */
    EXT_RESET_DEVICE,

    /* 管理设备 manage device
    */
    EXT_MANAGE_DEVICE,

    /* 强制关键帧 manage device
    */
    FORCE_KEY_FRAME,

    /*
    *  设备已连上服务器事件
    */
    EXT_DEVICE_ONLINE,
} CMD_ITEMS;

typedef struct
{
    int channel;                            /* 通道 */
    CMD_ITEMS cmd_id;                       /* 命令id参考CMD_ITEMS */
    char cmd_args[1024];                    /* 命令携带参数见上面注释 */
    int	 cmd_args_len;                      /* 数据长度 */
} CMD_PARAM_STRUCT;

typedef  enum
{
    MOVE_DETECT = 0x0004,                   /* 移动侦测 */
    VIDEO_LOST = 0x0008,                    /* 视频丢失 */
    VIDEO_SHELTER = 0x0010,                 /* 视频遮挡 */
    PRIVACY_NOTIYF = 0x0020,                /* 隐私报警 */
    AUDIO_DETECT = 0x0040,                  /* 声音侦测 */
    HUMAN_DETECT = 0x0080,                  /* 人形侦测 */
} ALARM_TYPE;

typedef enum
{
    ALARM_OFF,                              /* 报警结束 */
    ALARM_ON,                               /* 报警开始 */
    ALARM_PULSE                             /* 报警脉冲 */
} ULK_ALARM_STATE;

typedef  enum
{
    JPG = 0x0001,
    JPEG = 0x0002,
    BMP = 0x0003,
} PICTURE_TYPE;

typedef  enum
{
    ULK_NO_END = 0,
    ULK_END = 1,

    /* history ctrl request flags */
    ULK_HIST_NORMAL = 2, // video + audio
    ULK_HIST_CONTINUE = 3,
    ULK_HIST_VIDEO = 4, // video only
    ULK_HIST_AUDIO = 5, // audio only
} ULK_HISTORY_VIDEO_ENUM;

typedef  enum
{
    ULK_OFFLINE = 0,
    ULK_ONLINE = 1,
} ULK_CHNL_STATUS_ENUM;

typedef enum
{
    ULK_AUDIO_TYPE_AAC = 0,
    ULK_AUDIO_TYPE_G711A,
    ULK_AUDIO_TYPE_G711U,
} ULK_AUDIO_TYPE_ENUM;

typedef enum
{
    AYE_VAL_SD_NOOP = 0,
    AYE_VAL_SD_FORMAT,
    AYE_VAL_SD_NEXSIT,
    AYE_VAL_SD_OK,
    AYE_VAL_SD_BAD,
} AYE_TYPE_SD_STATUS;

typedef enum
{
    AYE_VAL_RECMODE_NOOP = 0,
    AYE_VAL_RECMODE_ALARM,
    AYE_VAL_RECMODE_NORMAL,
    AYE_VAL_RECMODE_VOICE,
    AYE_VAL_RECMODE_PLAN,
} AYE_TYPE_RECORD_MODE;

typedef struct preset_pos_info
{
    int index; /* start from 0 */
    int x; /* horizontal coordinates */
    int y; /* vertical coordinates */
    int z; /* lens zoom multiples */
} AY_PRESET_POS;

typedef struct
{
    char     MAC[32];                   /* MAC 地址 */
    uint32   OEMID;                     /* OEM ID,平台授权的厂商ID,如果没有,请填100827 */
    char     SN[32];                    /* 设备序列号,请填写已通过平台授权的20位序列号 */
    char     OEM_name[8];               /* 厂商OEM名称简称, 两个字符,用作序列号的前缀 */
    char     Model[65];                 /* 设备型号 */
    char     Factory[256];              /* 厂商名称,自定义(例如，悠络客、海康威视、大华等) */
} Dev_SN_Info;

typedef struct
{
    char    audio_type;                 /* 音频类型 ULK_AUDIO_TYPE_ENUM,目前只支持aac */
    char    is_end;                     /* 0 没有, 1 结束 */
    uint32  audio_seq;                  /* 流水号每个消息都有一个单独的序号 */
    char    *audio_buf;
    int	audio_len;
} ULK_Audio_Struct;

typedef struct
{
    uint8   channel_index;              /* 通道 */
    uint8   brightness;                 /* 亮度 */
    uint8   saturation;                 /* 饱和度 */
    uint8   Contrast;                   /* 对比度 */
    uint8   tone;                       /* 色度 */
} ULK_Video_Param_Ack;                  /* 图像参数上传 */

typedef struct
{
    uint8   channel_index;              /* 通道 */
    uint8   rate_index;                 /* 主子码流 0 子 1 主 */
    uint16  bit_rate;                   /* 码率 */
    uint16  frame_rate;                 /* 帧率 */
    uint16  video_quality;              /* 图像质量 >0 越小越好 */
} ULK_Video_Encode_Param_Ack;           /* 图像编码参数上传 */

typedef struct
{
    uint32  start;                      // the seconds from 0:0:0 in a day,scope: [0 ~ 86400],so the value of 0:0:0T would be 0.
    uint32  end;                        //  the seconds from 0:0:0 in a day,scope: [0 ~ 86400], so the value of 1:0:0T would be 3600.
} ULK_Alarm_Clock_struct;

typedef struct
{
    uint8   alarm_type;                 // 0 - motion alarm, 1 - audio alarm
    char    alarm_flag;                 /* 1 开启报警  0 关闭报警 */
    uint8   alarm_sensitivity;          // 0 - low, 1 - medium, 2 - high
    uint8   alarm_time_table_num;
    uint8   alarm_chn_mask[AY_MAX_CHANNEL_NUM]; /* 0 - enable, 1 - filter, default all enable */
    uint32  alarm_interval_mix;         /* 报警周期 */
    ULK_Alarm_Clock_struct  alarm_time_table[5];  /* 时间表 */
} ULK_Alarm_Struct;

typedef struct
{
    uint8   alarm_flag;     // alarm push switch: 0 - disable, 1 - enable
    uint8   alarm_time_table_num;   // the count of alarm time table setttings,max is 5.
    uint32  alarm_interval_mix;     //alarm push period, unit is second.
    ULK_Alarm_Clock_struct  alarm_time_table[5];    //alarm time table settings, see @ULK_Alarm_Clock_struct.
} AY_Alarm_Struct;

typedef enum
{
    Ulk802_11AuthModeOpen           = 0x00,
    Ulk802_11AuthModeWEPOPEN        = 0x01,
    Ulk802_11AuthModeWEPSHARE       = 0x02,

    Ulk802_11AuthModeWPAEAP         = 0x03,
    Ulk802_11AuthModeWPA2EAP        = 0x04,
    Ulk802_11AuthModeWPA1EAPWPA2EAP = 0x05,

    Ulk802_11AuthModeWPAPSKAES      = 0x06,
    Ulk802_11AuthModeWPAPSKTKIP     = 0x07,
    Ulk802_11AuthModeWPAPSKTKIPAES  = 0x08,

    Ulk802_11AuthModeWPA2PSKAES     = 0x09,
    Ulk802_11AuthModeWPA2PSKTKIP    = 10,
    Ulk802_11AuthModeWPA2PSKTKIPAES = 11,

    Ulk802_11AuthModeWPA1PSKWPA2PSKAES = 12,
    Ulk802_11AuthModeWPA1PSKWPA2PSKTKIP = 13,
    Ulk802_11AuthModeWPA1PSKWPA2PSKTKIPAES = 14,

    //Ulk802_11AuthModeMax,
    Ulk802_11AuthModeAuto,
} ULK_ENTICATION_MODE;

typedef struct
{
    PICTURE_TYPE  pic_type;
    uint8  *pic_data;
    uint32 pic_data_len;
} Picture_info;

typedef enum
{
    SDCARD_NONE,
    SDCARD_NORMAL,
    SDCARD_FORMATTING,
    SDCARD_BAD,
} SDcard_status;

typedef enum
{
    Media_Video_H264 = 0x01,
    Media_Video_H265 = 0x02
} Media_VideoCodecID;

typedef enum
{
    Media_Audio_AAC   = 0x01,
    Media_Audio_G711A = 0x02,
    Media_Audio_G711U = 0x03,
    Media_Audio_MP3   = 0x04
} Media_AudioCodecID;

/*
* @brief 初始化SDK环境
* @param[in] oem_info 设备厂商的OEM信息,必须设置
* @param[in] devattr  设备产品的基本功能属性,必须设置
* @param[in] logfile  SDK日志记录文件的绝对路径,如果不开启,设置为 NULL.
* @ret 0 - success, -1 - fail
* @note 新增接口,要求SDK版本>=1.5.0.用来替换原有的三个接口调用,避免产生调用顺序引入的问题:
*   Ulu_SDK_Enable_Debug + Ulu_SDK_Set_OEM_Info + Ulu_SDK_Init
*/
int Ulu_SDK_Init_All(Dev_SN_Info *oem_info,Dev_Attribut_Struct *devattr,const char *logfile);

/*
* @brief 设置已授权的20位设备SN号
* @param[in] sn20 20位已经授权通过的设备SN号
*
* @ret 0 - success, -1 - fail,sdk not-initialized or sn20 incorrect
*/
int Ulu_SDK_Set_20SN(const char *sn20);

/*
* 释放帧开辟缓冲区,结束时调用此函数
*/
void Ulu_SDK_DeInit(void);

/*
* 设置图像大小
*/
void Ulu_SDK_Set_Video_size(int width, int height);

/*
* 设置回调交互函数
*/
typedef void(*Interact_CallBack)(CMD_PARAM_STRUCT *args);
void Ulu_SDK_Set_Interact_CallBack(Interact_CallBack callback_fun);

/* 设置音频对讲回调函数 */
typedef void(*Audio_AAC_CallBack)(ULK_Audio_Struct *args);
void Ulu_SDK_Set_AudioAAC_CallBack(Audio_AAC_CallBack callback_fun);

/*
* 该函数是为了避免上面函数的命名产生歧义而增加的，为了能够支持多种对讲的音频数据格式而修改
* 函数设置的回调与上一个函数设置的是同一个,会相互覆盖,所以回调是以两者的后一次调用生效
*
* 建议使用该函数替代 @Ulu_SDK_Set_AudioAAC_CallBack 函数.
*/
typedef void(*Audio_Talk_CallBack)(ULK_Audio_Struct *args);
void Ulu_SDK_Set_Talk_CallBack(Audio_Talk_CallBack callback_fun);

/*
* 设置回调交互函数
* @ret  -1;设置失败 0;正常 -2;指针为空
*/
typedef void(*WIFI_CallBack)(char  *ssid, char *password, ULK_ENTICATION_MODE encrypt_type);
int Ulu_SDK_Set_WIFI_CallBack(WIFI_CallBack callback_fun);

/*
* 设置回调交互函数
*
* @param request_encrypt 	0 不需要,1 需要
* @param wireless_lan_name 网卡名称 如 ra0 wlan0
* @ret -1;设置失败 0;正常 -2;指针为空
*/
int Ulu_SDK_Set_WIFI_CallBack_Ext(WIFI_CallBack callback_fun, char request_encrypt, char *wireless_lan_name);

#ifndef WIN32
/*
* 启动监听模式
*
* @ret 0 正常; -1  没有设置回调接口 ;-2 无法找到对应的wifi网卡
*/
int Ulu_SDK_Start_Wifi_Monitor(void);

/*
* 停止监听模式
*
* @note 该函数是阻塞执行，最长阻塞120seconds，不能在WIFI_CallBack回调函数中调用该函数!!!!!
*/
void Ulu_SDK_Stop_Wifi_Monitor(void);

/*
* 通过截屏图片解析二维码获取wifi的
* 注意:push图片数据,前先设置wifi回调,图片格式支持jpg
* @param 传入需要进行解析的包含二维码的抓拍图片
* @note  二维码解析需要使用支持该功能的版本库, 如果函数返回-1表示版本库不支持二维码功能(简化版)
*/
int Ulu_SDK_Push_ScreenShot(Picture_info *pic_info);
#endif

/*
* @brief 发送用户数据 最大长度不能超过1024byte
* return:	-1  data length > 1024 ; 0 succeed
*/
int Ulu_SDK_Send_Userdata(uint8 *pdata, uint16 len);

/* 视频流数据上报,按照帧上报..I帧,P帧,音频帧等...
*
* @ret -4:波特率异常  -3:通道范围不对 -2:通道被屏蔽 -1:表示没有建立网络连接
* -6: 帧类型不对
* -7: 第一帧应该是I帧,会丢弃该帧
*  > 0 正常表示缓冲区使用数量
*/
int Ulu_SDK_Stream_Event_Report(Stream_Event_Struct *pEvent);

/*  抓拍图片上传
@ret  -5:通道范围不对  -4:没有连接  -3:周期太短 -2:时间范围不允许 -1:服务器禁止报警.0 正常
@note 图片最大不能超过210k
*/
int Ulu_SDK_Screen_Shot_Upload(Screen_Shot_Struct *ppic);

/*  报警上传扩展接口
*   @ret -5:通道范围不对  -4:没有连接  -3:周期太短 -2:时间范围不允许 -1:服务器禁止报警.0 正常
*/
int Ulu_SDK_Alarm_Upload_Ex(int channel, ALARM_TYPE  alarm_type);

/*  报警上传扩展时间接口
*   @ret -5:通道范围不对  -4:没有连接  -3:周期太短 -2:时间范围不允许 -1:服务器禁止报警.0 正常
*/
int	Ulu_SDK_Alarm_Upload_V3(int channel, ALARM_TYPE  alarm_type, uint32 alarm_time);

/*  报警上传扩展报警状态接口
*   @ret -5:通道范围不对  -4:没有连接  -3:周期太短 -2:时间范围不允许 -1:服务器禁止报警.0 正常
*/
int	Ulu_SDK_Alarm_Upload_V4(int channel, ALARM_TYPE alarm_type, uint32 alarm_time, ULK_ALARM_STATE alarm_state);

int Ulu_SDK_Alarm_Stream_Start(int channel);

int Ulu_SDK_Alarm_Stream_End(int channel);

/* 上报通道状态
-1 通道号错误 0 正常
*/
int Ulu_SDK_ChannelStatus_Event(int channel, ULK_CHNL_STATUS_ENUM status);

/*
获取最近上传发送速度
send_speed 	发送数据的速度
send_ok_speed  实际发送成功的速度
注:计算周期4s内的数据统计byte
*/
void Ulu_SDK_Get_Upload_Speed(uint32 *send_speed, uint32 *send_ok_speed);

/*
获取流服务器连接状态
1 -> 连接正常, -2 正在连接 -1 没有连接
*/
int Ulu_SDK_Get_Connect_Status(void);

/*
* 获取设备ID信息,如果设备是第一次注册,需要等待延时5s,等待设备注册生成id,
* 否则可能会返一个无效的id 
*/
char* Ulu_SDK_Get_Device_ID(void);

/*
* 推送本地录像帧数据
*
* @ret -4:波特率异常  -3:通道范围不对 -2:通道被屏蔽 -1:表示没有建立网络连接
* -6: 帧类型不对
* -8: 发送缓冲区剩余空间不足以存放该帧,请尝试重新发送
* > 0 正常表示缓冲区使用数量
*/
int	Ulu_SDK_History_Frame_Send(Stream_History_Frm_Struct *pfrm_data, ULK_HISTORY_VIDEO_ENUM is_end);

/*
* 历史视频请求时间点数据为空时通知到客户端
*/
int Ulu_SDK_History_Srch_Rslt_None(uint8 channel);

/*
* 获取设备上线状态
*
* @ret 1 - Device is online, 0 - offline, -1 - regist has fail
* @note require sdk version >= 1.5.0.
*/
int Ulu_SDK_Get_Device_Online(void);

/*
* 复位流服务器的连接
*
* @ret None.
* @note 当配置网络地址,更新网络参数影响到网络连接时,在更新成功后,请调用该接口以便尽快恢复设备与服务器的连接.
*/
void Ulu_SDK_Reset_Stream_Links(void);

/*
* 发送历史视频时间列表
*
* @ret 0 - success
* @note require sdk version >= 1.5.0.
*/
int Ulu_SDK_History_List_Send(HistoryListQuery_Struct *pEvent);

/*
* 设置设备视频的RTSP观看网址
*
* @param[in] url rtsp video url.
* @ret None.
* @note require sdk version >= 1.5.0.
*/
void Ulu_SDK_Set_Rtsp_Url(const char *url);

/*
* 获取SDK的发布版本号
*
* @ret return release version id, for vA.B.C, id = (A<<16)|(B<<8)|C.
* @note require sdk version >= 1.5.0.
*/
int Ulu_SDK_Get_Version(void);

/*
*	获取设备报警控制参数
*  @param[in] type 0 - motion alarm, 1 - audio alarm
*  @param[in,out] pctrl return the alarm ctrl parameters if found the alarm type
*  @ret return 0 when found, or return -1.
*/
int	Ulu_SDK_Get_Alarm_Ctrl(uint8 type,ULK_Alarm_Struct *pctrl);

/*
* 复位解绑设备 reset to unbind device
*
* @ret 0 - success, -1 - param error,-2 - network error,-3 - decode error, -4 - respnse error,-5 - code error
*/
int Ulu_SDK_Reset_Device(void);

/*
* 上报设备TF卡状态与录像模式
*
* @param[in] status device TF card status,see @AYE_TYPE_SD_STATUS define
* @param[in] mode device record mode value,see @AYE_TYPE_RECORD_MODE define
* @ret 0 - success, -1 - when stream link isnot connected.
*/
int Ulu_SDK_Device_Record_Report(AYE_TYPE_SD_STATUS status,AYE_TYPE_RECORD_MODE mode);

/*
* 返回预置位结果集 response query preset position result
* @param[in] preset[] all preset position result
* @param[in] num count of preset position result
*/
int Ulu_SDK_EXT_Response_All_Preset(AY_PRESET_POS preset[],int num);

/*
* 返回预置位操作结果 response control preset result
* @param[in] index preset id,start from 0
* @param[in] result control preset result,0 - fail, 1 - success
*/
int Ulu_SDK_EXT_Response_Preset_Result(int index,int result);

/*
* 返回视频翻转信息 response video turn parameters
* @param[in] hturn horizontal turn value, 0: close 1: turn 180 degree
* @param[in] vturn vertival turn value, 0: close 1: turn 180 degree
* @ret 0 - success, -1 - fail
*/
int Ulu_SDK_EXT_Response_Image_Turn(int hturn, int vturn);

/*
* 返回移动侦测敏感度 response motion detect sensitivity
* @param[in] value  sensitivity value,10 - low 20 - medium 30 - high
* @param[in] enable 0 - disable,1 - enable
* @ret 0 - ok, -1 - fail
*/
int Ulu_SDK_EXT_Response_Mdsense(int value,int enable);

/*
* 返回声音侦测敏感度 response audio detect sensitivity
* @param[in] value  sensitivity value,10 - low 20 - medium 30 - high
* @param[in] enable 0 - disable,1 - enable
* @ret 0 - ok, -1 - fail
*/
int Ulu_SDK_EXT_Response_Adsense(int value,int enable);

/*
* 返回隐私报警通知 response private alarm notification
* @param[in] msg private alarm message content,max length < 64
* @ret 0 - ok, -1 = fail
*/
int Ulu_SDK_EXT_Response_Private_Alarm(const char *msg);

/*
* 返回侦测记录 response alarm detect record
* @param[in] start alarm record start time
* @param[in] end alarm record end time
* @param[in] bitmap alarm record bitmap, one bit for one minute
*/
int Ulu_SDK_EXT_Response_Detect_Record(uint32 start,uint32 end,int bitmap);

/*
* 返回SD卡状态结果 response sdcard status information
* @param[in] status sdcard status
* @param[in] record record enable flag, 0 - disable, 1 - enable normal record, 2 - enable alarm record
* @param[in] total_v sdcard total volume,unit is MB
* @param[in] free_v sdcard free volume,unit is MB
*/
int Ulu_SDK_EXT_Response_Sdcard_Result(SDcard_status status,int record,int total_v,int free_v);

/*
*   获取系统时间戳.毫秒单位
*/
uint64 Ulu_SDK_GetTickCount(void);

#ifndef WIN32
///////////////////////////////// Deprecated APIs //////////////////////////////////////////////////////////
/*
*   启动自动时间同步.默认启用
*   下发回调命令：TIME_SYN, 上层处理
*/
void AYAPI_ATTR Ulu_SDK_Enable_AutoSyncTime(void);

/*
*   禁止自动时间同步
*   取消回调命令, 可以不需要该接口,直接忽略掉回调命令TIME_SYN就可以.
*/
void AYAPI_ATTR Ulu_SDK_Disable_AutoSyncTime(void);

/*	报警上传: 通道1的报警上传
*	@note: 建议不再使用该接口，使用Ulu_SDK_Alarm_Upload_Ex接口替代.
*/
void AYAPI_ATTR Ulu_SDK_Alarm_Upload(ALARM_TYPE alarm_type);

/*
* NVR初始化:该函数已经不再需要
* 该函数已经不做任何工作，仅仅为了兼容接口而保留
*/
int	AYAPI_ATTR Ulu_SDK_NVR_Init(int  blocknums, int ts_mem_size);

/*  图像参数上传 暂未实现
*/
void AYAPI_ATTR Ulu_SDK_Param_Vedio_Upload(ULK_Video_Param_Ack *video_param);

/*
    图像编码参数上传 暂未实现
*/
void AYAPI_ATTR Ulu_SDK_Code_Param_Vedio_Upload(ULK_Video_Encode_Param_Ack *video_param);

/*
* 看门狗初始化,用于外部进程检测网络库是否正常的,厂商应该实现自己的看门狗机制
*
* @note call this API in the watchdog process,another process.
*/
int AYAPI_ATTR Ulu_SDK_Watchdog_init(void);

/*
* 获取安眼设备SDK工作状态 get anyan device sdk work status in the watchdog process
*
* @ret 1 - anyan sdk works ok, 0 - anyan sdk has some exception, -1 - watchdog init fail.
* @note
*	1. call this API in the watchdog process,another process.
*	2. 最短检测周期 > 15s ,<15s 没有意义.函数会直接返回1
*/
int AYAPI_ATTR Ulu_SDK_Get_ulu_net_status(void);

/*
*  启动调试模式 若调用此函数启动调试模式，同时生成日志文件，参数是日志文件的完整文件名，例如:/tmp/ulk.log
*  log_file_full_name:日志文件路径+文件名
*  建议使用：Ulu_SDK_Init_All(...) 设置, 20150910
*/
void AYAPI_ATTR Ulu_SDK_Enable_Debug(const char *log_file_full_name);

/*
    设置OEM Info,厂商在此设置自己的厂商名、设备型号、厂商ID、厂商名等信息
    建议使用：Ulu_SDK_Init_All(...) 设置, 20150910
*/
void AYAPI_ATTR Ulu_SDK_Set_OEM_Info(Dev_SN_Info  *Oem_info);

/*
    初始化函数,为帧开辟缓冲区.. 0成功 -1失败
    建议使用：Ulu_SDK_Init_All(...) 设置, 20150910
*/
int AYAPI_ATTR  Ulu_SDK_Init(Dev_Attribut_Struct  *attr);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ANYAN_DEVICE_SDK_H__ */
