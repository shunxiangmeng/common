#ifndef __AY_LAN_H__
#define __AY_LAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aystream.h"
#include "channel.h"
#include "ayqueue.h"

#define LAN_STREAM_PORT	9100

enum client_object_status
{
    AYE_CLIOBJ_STATUS_INIT = 0,
    AYE_CLIOBJ_STATUS_LINK,
    AYE_CLIOBJ_STATUS_LOGIN,
    AYE_CLIOBJ_STATUS_LIVE,
    AYE_CLIOBJ_STATUS_LOGOUT,
    AYE_CLIOBJ_STATUS_EXIT,
};
typedef struct ay_client_object
{
    int fd;
    int status;
    time_t expiretime;
    data_recv_struct recvbuf;
    Chnl_ctrl_table_struct chnfilter[MAX_CHANNEL_NUM];

    struct ay_client_object *pnext;
}st_ay_client_object;

typedef struct ay_device_object
{
    int handle;
    int status;
    int maxfd;
    fd_set rdset;
    fd_set wrset;
    st_ay_client_object *pclients;
    Chnl_ctrl_table_struct streamFilter[MAX_CHANNEL_NUM];

    Chnl_Buf_Queue_head streamBuf;
    Msg_Buf_Queue_head cmdUpQueue;
    Msg_Buf_Queue_head cmdDownQueue;
    Msg_Buf_Queue_head talkQueue;
}st_ay_device_object;


int ay_response_client_login(int fd,device_id_t did);
void *lan_stream_thread(void *arg);


#ifdef __cplusplus
}
#endif

#endif
