#ifndef __AY_SDK_H__
#define __AY_SDK_H__

#include "define.h"
#include "protocol_device.h"
#include "Anyan_Device_SDK.h"
#include "aylan.h"
#include "monitor.h"

#define IS_LIVE_FRAME(type) ((type==CH_I_FRM) || (type==CH_P_FRM) || (type==CH_AUDIO_FRM)) 
#define IS_HIST_FRAME(type) ((type==CH_HIS_I_FRM) || (type==CH_HIS_P_FRM) || (type==CH_HIS_AUDIO_FRM)) 

enum ay_error
{
    AY_ERROR_OK = 0,
    AY_ERROR_CHN = -10,
    AY_ERROR_TOKEN = -11,
    AY_ERROR_MEMORY = -12,
    AY_ERROR_ID = -13,
    AY_ERROR_PARAM = -14,
    AY_ERROR_NETWORK = -15,
    AY_ERROR_EXCGKEY = -16,// key doesn't exist
    AY_ERROR_PRIENCRY = -17, // private encry error
    AY_ERROR_PRIDECRY = -18, // private decry error
    AY_ERROR_MSGTYPE = -19, // unknown message type
    AY_ERROR_MSGFULL = -20, // unknown message type
    AY_ERROR_RUNING = -21, // task is runing,wait
    AY_ERROR_STREAM = -22, // stream ip reassigned

    AY_ERROR_MAX = -99,
};

typedef struct 
{
    uint32  ip;
    uint16  port;
    uint16  flag;  // 0x00 - udp mode, 0x01 - tcp mode 

    uint8   state; // 0 - send, 1 - recv
    uint8   rtcnt; // retry count when fail 
    uint16  wtout; // next timeout to wait, unit is second 
    int	    tsock; // short tcp socket
    struct timespec last_sndt;
}st_ay_entry;

typedef struct ay_entry_ctrl
{
    uint16	    num;	
    st_ay_entry	    addr[ENTRY_SERVER_MAX_NUM];
}st_ay_entry_ctrl;

struct debug_log_cache
{
	pthread_mutex_t mutex;
    char *pbuf;
    int cap;
    int size;
    int woffset;
    int roffset;
};
typedef struct ay_debug_ctrl
{
    char pathfile[256];
    FILE *log_fd;
    char en_flag;
    int max_fsize;
    struct debug_log_cache log_cache;
}st_ay_debug_ctrl;

typedef struct ay_callback_ctrl
{
    Audio_AAC_CallBack  pAudioCbfc;
    WIFI_CallBack	pWifiCbfc;
    Interact_CallBack   pCmdCbfc;
    Interact_CallBack   pOemCbfc; /* handle OEM Private Cmd bythrough TransparentWay if NEED */
}st_ay_callback_ctrl;

struct ay_thread_unit
{
    pthread_t tid;
    char name[64];
};
typedef struct ay_thread_ctrl
{
    int num;
    struct ay_thread_unit tasks[16];
}st_ay_thread_ctrl;

typedef Dev_info_struct st_ay_dev_info;

typedef struct ay_talk_ctrl
{
    Audio_Talk_Buf_Struct   talk_buf;
    Audio_Talk_Ctrl_Struct  talk_ctrl;
}st_ay_talk_ctrl;

typedef struct ay_stream_ctrl
{
    sem_t	    data_ok_sem;
    uint32	    status;
    uint32	    snd_report_cnt[2];// [0] - from boot ok,[1] - from last login ok
    uint32	    rcv_report_cnt[2];// [0] - from boot ok,[1] - from last login ok
    Connect_Info    conns[MAX_STREAM_THREAD_NUM];
}st_ay_stream_ctrl;

typedef struct ay_wifi_ctrl
{
    char    ulk_ssid[33];
    char    ulk_stapassword[33];
    uint8   ulk_encrypto;
    char    wireless_name[16];
    char    is_request_encrypt;
}st_ay_wifi_ctrl;

typedef struct ay_history_ctrl
{
    struct {
	char		his_flag;// 0 - TS Packframe, 1 - private Packframe
	char 		his_enable;
	char 		his_request_id[256];
	uint32 		his_req_id_length;
    } his_info[AY_MAX_CHANNEL_NUM];

    ULK_Hist_Rslt_Ack	his_ack[AY_MAX_CHANNEL_NUM];
}st_ay_history_ctrl;

#define AY_MAX_DNSIP_NUM 10
typedef struct ay_dns_ctrl
{
    st_ay_net_addr dnsok; // dns can serve
    st_ay_net_addr dnsips[AY_MAX_DNSIP_NUM]; // dns list: http://192.168.18.139:8081/dnslist
    struct { // dns nslookup:  http://192.168.18.139:8081/nslookup
	char name[64];
	st_ay_net_addr lastip;
	st_ay_net_addr defip;
    } host[5];
}st_ay_dns_ctrl;

#define SDK_FLAG_UPLOAD_SNAP	    0x01
#define SDK_FLAG_UPLOAD_MD_STREAM   0x02
#define SDK_FLAG_CLOUD_STREAM	    0x04
#define SDK_FLAG_UPLOAD_HUMAN_STREAM	0x08
#define SDK_FLAG_UPLOAD_HD_STREAM   0x10

typedef struct ay_sdk_instance_ctrl
{
    st_ay_debug_ctrl debug;
    st_ay_callback_ctrl cbfuncs;
    st_ay_thread_ctrl threads;
    st_ay_dev_info devinfo;
    st_ay_talk_ctrl talk;
    st_ay_stream_ctrl stream;
    st_ay_wifi_ctrl wifi;
    st_ay_device_object devobj[2];// 0 - wan stream, 1 - lan stream
    st_ay_history_ctrl history;
    st_ay_dns_ctrl  dnsctrl;

    ULK_Alarm_Struct	alarm_ctrl; 
    pthread_mutex_t frame_report_lock;
    uint32  net_stream_upload_bytes;
    uint32  exit_flag;
    uint32  inst_flag;
	uint32  alarm_state;
    time_t  mdcloud_start;
	time_t  human_detect_start;
	time_t	alarm_time;
	pthread_mutex_t stream_flag_lock;
}st_ay_sdk_instance, *ay_sdk_handle;

ay_sdk_handle sdk_get_handle(int id); /* default 0 */
ay_sdk_handle sdk_init_instace(int id); /* default 0 */

void  notify_alarm_config_update(ULK_Alarm_Struct *palarm,int num);
void  call_back_error_info(char *msg);
int  Computer_Net_UploadSpeed(uint32 tlen);
uint32 get_device_flag(st_ay_dev_info *pdevinfo);

void ay_parse_oem_cmd(CMD_PARAM_STRUCT *args);

int ay_init_device_object(st_ay_device_object *pdevobj,int nums,int upnums,int downnums,int talknums);
int ay_deinit_device_object(st_ay_device_object *pdevobj);
int aystream_add_frame(st_ay_device_object *pdevobj,Stream_Event_Struct *pEvent);

extern ay_sdk_handle ay_psdk;

#endif

