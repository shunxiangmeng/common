#ifndef __AY_STREAM_H__
#define __AY_STREAM_H__ 

#include "typedefine.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define  MAX_STREAM_MSG_LEN 	    1000
#define  MAX_FRAME_MSG_LEN 	    (MAX_STREAM_MSG_LEN - 200)
#define	 MAX_STREAM_THREAD_NUM      1//流服务连接数量

#define	 FRAME_SNED_SIZE            (8*1024)
#define	 FRAME_RECV_SIZE            (8*1024)
#define	 FRAME_AUDIO_FRM_SIZE       (8*1024)

#define CONN_TYPE_STREAM_LIVE 0x0001
#define CONN_TYPE_MSGCMD      0x0002
#define CONN_TYPE_STREAM_HIST 0x0004

    typedef enum 
    {
	AYE_VAL_STREAM_STATUS_DISCONNECT = 1,
	AYE_VAL_STREAM_STATUS_CONNECTING,
	AYE_VAL_STREAM_STATUS_CONNECTED,
	AYE_VAL_STREAM_STATUS_WAITREQ = 10, // from stream server/client
	AYE_VAL_STREAM_STATUS_WAITDAT, // device report stream
	AYE_VAL_STREAM_STATUS_SENDDAT,
    }aye_type_stream_status;
    typedef struct
    {      
	size_t	data_len;
	char    data_buf[FRAME_RECV_SIZE+512];	
    }data_recv_struct;

    typedef struct
    {
	int   id;
	int   type;
	int   socket_fd;
	int   status;
	int   keepidle;// default 40secs
	data_recv_struct p_recv;
	pthread_t pth_id;
	sem_t	conn_ok_sem;
	pthread_mutex_t fd_lock;		
	st_ay_net_addr myaddr;
    } Connect_Info;

    void init_stream_env(void);
    void exit_stream_env(void);

    int  aystream_get_status(void);
    int aystream_reset_conns(void);

    void *aystream_send_thread(void* arg);
    void *aystream_recv_thread(void* arg);

    int ay_recv_stream(int sockfd, data_recv_struct *p_fd_buf,char decry,int (*onFailDeal)(int ));
    int ay_send_stream(int sockfd, char *buf, int dtlen, char encry,int (*onFailDeal)(int ));

#ifdef __cplusplus
};
#endif

#endif
