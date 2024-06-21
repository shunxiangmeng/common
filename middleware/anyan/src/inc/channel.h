#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "typedefine.h"
#include "Anyan_Device_SDK.h"

#define	 MAX_CHANNEL_NUM           AY_MAX_CHANNEL_NUM 
#define  TS_PACKAGE_LEN		    (8*1024) // History Frame Send Size
#define	 SCREENSHOT_SUB_SIZE        (950)

typedef struct
{
    int     frm_id_all;             /* 总id视频和音频总数 */
    int     frm_vedio_id;           /* 视频帧id */
    int     frm_audio_id;           /* 音频帧id */

    uint32  last_time_stamp;        /* 上一帧的ms */
    uint32  time_second_ms_base;
    uint32  time_ms_ref;

    uint32  last_time_stamp_audio;  /* 上一帧的ms */
    uint32  time_second_ms_base_audio;
    uint32  time_ms_ref_audio;

    uint32  now_send_I_ts;          // second
    int first_I_frame_required;     /* we required I frame when open stream , set to 1 */
    uint32  last_I_time_stamp;      /* 上一帧的ms */
    uint32  latest_gop_gap;         // One Gop(I-I) Gap Time,unit ms
} Chnl_RunNum_Struct;

typedef struct
{
    /*
     * video and audio filter flag(low 8bits),0x00 - none, 0x01 - video 0x02 - audio, 0x03 - all
     * cmd from who(high 8bits): 0x01 - server cmd, 0x02 - sdk internal cmd(mdcloud storage) 
     */
#define CMD_FROM_SERVER	    0x0100
#define CMD_FROM_MDCLOUD    0x0200
#define CMD_FROM_CLIENT	    0x0400
#define CMD_FROM_HUMAN		0x0800
    uint16   	bit_r[4];
    uint16	bit_his_r;// same with av_filter
    Chnl_RunNum_Struct  frm_id[4];		
    Chnl_RunNum_Struct  frm_id_his;
}Chnl_ctrl_table_struct;

typedef struct
{    	
    char	user_name[50];  /* 用户名最长 20 */
    uint32	from;
    uint32	type;
    uint32	seq;
    uint32	audio_total_len;    		
}Audio_Info_struct;

typedef struct
{
    uint32 seq;             /* 流水号每个消息都有一个单独的序号 */
    int  len;
    char buf[128*1024];
} Audio_Talk_Buf_Struct;

typedef struct 
{
    char        flag;       /* 1 占用, 0  空闲 */
    sem_t       play_ok;
    uint32      seq_cur;
    ULK_Audio_Struct    ulk_buf;
} Audio_Talk_Ctrl_Struct;

typedef struct
{
    uint8   channel_index;
    uint16  rate;
    uint32  ts_ts;
    uint32  ts_size;
    uint32  ts_duration;
} ULK_Hist_Rslt_Ack;

typedef struct
{
    int     flag_pl ;       /* 是否设置好了seek */
    int     tm_ct_out;      /* 倒计时关闭查询文件 */
    uint32  ts_last_end;    /* 删一次结束点 */
    uint32  ts_timestamp_start;/* 开始时间点 */
    uint32  ts_last_tm;     /* 上次时间点 */
    uint8   *p_ts_data;     //ts_data
    uint32  ts_duration;	/* 打包后的ts大小 */
    int     chnl_ts;
    uint16  hist_rate;
    int     his_status;     /* 上传状态 0 未使用 1正在使用 */
    unsigned long   time_stamps_frm;
    pthread_mutex_t his_lock;
} HISTORY_REQ_INFO;


int Get_All_chnl_status(Chnl_ctrl_table_struct *ptable);
void Clean_All_Chnl_Second_Base(Chnl_ctrl_table_struct *ptable);

int PTZ_Channel_ctrl(uint8 ch,uint32  ptzctrl,uint32 steps);
int add_channel_cmd(uint32 cid,uint32 chnlnum, uint32 enable, uint32 rate,uint32 cmd_from);

int copy_talk_data(uint32 audio_total_len,uint32  audio_offset,uint32  audio_len,uint8 *audio_data);
void deal_talk_thread(int arg);

int History_Srch_Rslt_Ack(uint32 ts_size,uint8 chn);
int History_List_Upload(HistoryListQuery_Struct *his_list_report, uint32 flag);

void upload_screen_snap(int arg);

int is_chn_filter(Chnl_ctrl_table_struct *ptable,int chn,int frm_type,int brate);
void update_send_I_ts(Chnl_ctrl_table_struct *ptable,int chn,int rate,uint32 ts);
int compare_send_I_ts(Chnl_ctrl_table_struct *ptable,int chn,int rate,uint32 ts);

int disable_stream_request(Chnl_ctrl_table_struct ptable[],int from);

#endif
