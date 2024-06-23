#ifndef __AY_QUEUE_H__
#define __AY_QUEUE_H__ 

#include "protocol_device.h"

#define  CMD_BUF_SIZE       		(1024+30)

typedef struct frm_buf_list
{
    uint32  msg_serial_num;     /* 消息流水号 */
    int	    channelnum;         /* 通道号 */
    uint16  frm_rate;           /* 帧率 */
    uint16  frm_type;           /* 帧类型 */	
    uint16  media_video_type;   /* 视频编码格式 */
    uint16  media_audio_type;   /* 音频编码格式 */
    uint32  frameid;            /* 帧号+包含音频 */ 
    uint32  frm_av_id;          /* 音频或者视频 */
    uint32  frm_ts_sec;	        /* 时间戳s */
    uint32  frm_ts;             /* 时间戳ms */
    uint32  crc32_hash;         /* 校验值 */
    int     whole_len;          /* 帧总长度 */	
    int     offset;             /* 标识数据块是一个帧偏移点 */
    int     len;                /* 数据长度 */
    char    *buf;               /* 通道缓冲区 */
    struct frm_buf_list *pnext;
} frm_buf_struct;

struct frm_buf_queue
{
    frm_buf_struct *head;
    frm_buf_struct *tail;
};

typedef struct
{
    uint32          total_nums;
    uint32          frm_nums;
    uint32          last_serial_num;
    pthread_mutex_t q_lock;
    sem_t           q_sem;

    frm_buf_struct  *phead;// used in destroy
    struct frm_buf_queue    readQueue;
    struct frm_buf_queue    writeQueue;

    uint32          need_Iframe_flag[MAX_CHANNEL_NUM];
} Chnl_Buf_Queue_head;	
	
typedef struct msg_buf_s
{
    uint32          msg_serial_num;
    int             len;
    char            buf[CMD_BUF_SIZE];
    struct msg_buf_s    *pnext;
} msg_buf_struct;

typedef struct
{
    uint32          total_nums;
    uint32          frm_nums;
    uint32          last_serial_num;
    pthread_mutex_t q_lock;
    sem_t           q_sem;
    msg_buf_struct  *q_p_end;
    msg_buf_struct  *q_list;
} Msg_Buf_Queue_head;

int ay_init_frm_buf(Chnl_Buf_Queue_head *pHead,unsigned buf_num);
int ay_add_chnl_frame(Chnl_ctrl_table_struct *ptable,Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pdata);
int ay_get_frame_buf_free(Chnl_Buf_Queue_head *pQHead);
int ay_get_frame_buf_first(Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pinfo);
int ay_get_frame_buf_first_timedwait(Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pinfo,int ms);
int ay_del_frame_buf_first(Chnl_Buf_Queue_head *pQHead,uint32  frmid);
int ay_destroy_frm_buf(Chnl_Buf_Queue_head *pQHead);

int init_msg_queue(Msg_Buf_Queue_head *queue, int capacity);
int add_msg_queue_item(Msg_Buf_Queue_head *queue,char *data,int len,int flag);
int get_msg_queue_item(Msg_Buf_Queue_head *queue,char *pmsg_out);
int del_msg_queue_first(Msg_Buf_Queue_head *queue);
int destroy_msg_queue(Msg_Buf_Queue_head *queue);

///////////////////////////////////////////////// Just Wrapper APIs ////////////////////////////////////////////////
//
#define ADD_MSG_FLAG_COVER 0x00 // default
#define ADD_MSG_FLAG_FHINT 0x01 // full hint
int  add_msg_cmd(char *data, int len,int flag);
int   Msg_Cmd_Add_queue(char *data, int len);
int   Msg_Cmd_Get_queue(char *pmsg_out, uint32  *msg_serialnum);
int   Msg_Cmd_Get_queue_timedwait(char *pmsg_out, int ms);
int   Msg_Cmd_Del_queue_first(void);

int  Msg_Cmd_Add_down_queue(char *data, int len);
int  Msg_Cmd_Get_down_queue(char *pmsg_out);
int  Msg_Cmd_Del_queue_down_first(void);

int  Audio_msg_Add_queue(char *data, int len);
int  Audio_msg_Get_queue(char *pmsg_out);
int  Audio_msg_Del_queue_first(void);

///////////////////////////////////////////////// Just Wrapper APIs ////////////////////////////////////////////////

#endif
