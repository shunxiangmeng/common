#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#ifdef WIN32
#include "windows-porting.h"
#else
#include <netinet/tcp.h>
#endif

#include "protocol_exchangekey.h"
#include "cdiffie_hell_man.h"
#include "monitor.h"
#include "http_base64.h"
#include "threadpool.h"

#include "ayutil.h"
#include "ayinit.h"
#include "ayqueue.h"
#include "aystream.h"
#include "ayentry.h"
#include "ay_sdk.h"
#include "version.h"

static int   Get_frame_buf_list_first(frm_buf_struct *pinfo)
{
    return ay_get_frame_buf_first(&sdk_get_handle(0)->devobj[0].streamBuf,pinfo);
}
int  Get_frame_buf_list_first_timedwait(frm_buf_struct *pinfo,int ms)
{
    return ay_get_frame_buf_first_timedwait(&sdk_get_handle(0)->devobj[0].streamBuf,pinfo,ms);
}
static int  Del_frame_buf_list_first(uint32  frmid)
{
    return ay_del_frame_buf_first(&sdk_get_handle(0)->devobj[0].streamBuf,frmid);
}

void init_stream_env(void)
{
    int i;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;

    InitDHDataList(); // init DiffieHellMan datalist
    sem_init (&pstream->data_ok_sem, 0, 0);
    for(i=0;i<2;i++)
    {
		pstream->snd_report_cnt[i] = 0;
		pstream->rcv_report_cnt[i] = 0;
    }
    for(i = 0; i < MAX_STREAM_THREAD_NUM; i++)
    {
		pstream->conns[i].id = i;
		pstream->conns[i].socket_fd = -1;
		pstream->conns[i].keepidle = 40;
		pstream->conns[i].p_recv.data_len = 0;
		pstream->conns[i].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
		if(i%2)
		{// 1,3,5 ...
			pstream->conns[i].type = CONN_TYPE_STREAM_LIVE|CONN_TYPE_STREAM_HIST;
		}
		else
		{// 0,2,4 ...
			pstream->conns[i].type = CONN_TYPE_MSGCMD;
		}
		memset(&pstream->conns[i].myaddr,0,sizeof(st_ay_net_addr));
		sem_init (&pstream->conns[i].conn_ok_sem, 0, 0);
		pthread_mutex_init(&pstream->conns[i].fd_lock, NULL);
    }
    pstream->status = AYE_VAL_STREAM_STATUS_WAITREQ;
    DEBUGF("stream env init finish.\n");
}

void exit_stream_env(void)
{
    int i;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;

    aystream_reset_conns();
    for(i=0;i<2;i++)
    {
		pstream->snd_report_cnt[i] = 0;
		pstream->rcv_report_cnt[i] = 0;
    }
    for(i = 0; i < MAX_STREAM_THREAD_NUM; i++)
    {
		memset(&pstream->conns[i].myaddr,0,sizeof(st_ay_net_addr));
		sem_destroy(&pstream->conns[i].conn_ok_sem);
		pthread_mutex_destroy(&pstream->conns[i].fd_lock);
    }
    pstream->status = AYE_VAL_STREAM_STATUS_WAITREQ;
    sem_destroy(&pstream->data_ok_sem);
}

int aystream_get_status(void)
{
    if(sdk_get_handle(0)==NULL) return -1;

    int i = 0,status = -1;// disconnect
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    for(i=0;i<MAX_STREAM_THREAD_NUM;i++)
    {
		pthread_mutex_lock(&pstream->conns[i].fd_lock);
		switch(pstream->conns[i].status)
		{
		case AYE_VAL_STREAM_STATUS_CONNECTING:
			status = -2;break;
		case AYE_VAL_STREAM_STATUS_CONNECTED:
			status = 1;break;
		}
		pthread_mutex_unlock(&pstream->conns[i].fd_lock);
		if(status == 1) break;
    }
    return status;
}

int aystream_reset_conns(void)
{
    int i;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    for ( i = 0; i < MAX_STREAM_THREAD_NUM; i++ )
    {
		pthread_mutex_lock(&pstream->conns[i].fd_lock);
		if(pstream->conns[i].socket_fd > 0)
		{
			Set_Connect_status(pstream->conns[i].socket_fd, 0);
			closesocket(pstream->conns[i].socket_fd);
			pstream->conns[i].p_recv.data_len = 0;
			pstream->conns[i].socket_fd = -1;
			pstream->conns[i].keepidle = 40;
			pstream->conns[i].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
		}
		pthread_mutex_unlock(&pstream->conns[i].fd_lock);
    }
    STATUS_CLR(STREAM_LOGIN);
    pstream->snd_report_cnt[1] = 0;
    pstream->rcv_report_cnt[1] = 0;
    return 0;
}

int aystream_set_keepidle(int fd,int idle)
{
    int i;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    for (i = 0; i < MAX_STREAM_THREAD_NUM; i++)
    {
		pthread_mutex_lock(&pstream->conns[i].fd_lock);
		if(pstream->conns[i].socket_fd > 0 && pstream->conns[i].keepidle!=idle)
		{
			if(fd == 0 || fd == pstream->conns[i].socket_fd)
			{// 0 - all
				setsockopt(pstream->conns[i].socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&idle , sizeof(idle));
				pstream->conns[i].keepidle = idle;
			}
		}
		pthread_mutex_unlock(&pstream->conns[i].fd_lock);
    }
    return 0;
}
static int Get_socketfd(int parr[],data_recv_struct *precvbuf[])
{
    int i,j = 0;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;

    for ( i = 0 ; i < MAX_STREAM_THREAD_NUM; i++ )
    {
		pthread_mutex_lock(&pstream->conns[i].fd_lock);
		if(pstream->conns[i].socket_fd > 0)
		{
			parr[j] = pstream->conns[i].socket_fd;
			if(precvbuf) precvbuf[j] = &pstream->conns[i].p_recv;
			j ++;
		}
		pthread_mutex_unlock(&pstream->conns[i].fd_lock);
    }
    return j;
}

#if 0
static int Get_socketfd_bytype(int parr[],int type)
{
    int i,j = 0;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    for ( i = 0 ; i < MAX_STREAM_THREAD_NUM; i++ )
    {
	pthread_mutex_lock(&pstream->conns[i].fd_lock);
	if(pstream->conns[i].socket_fd > 0 && (pstream->conns[i].type&type)==type)
	{
	    parr[j++] = pstream->conns[i].socket_fd;
	}
	pthread_mutex_unlock(&pstream->conns[i].fd_lock);
    }
    return j;
}
#endif

static int Del_socketfd(int fd)
{
    int i;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    for ( i = 0; i < MAX_STREAM_THREAD_NUM; i++ )
    {
	pthread_mutex_lock(&pstream->conns[i].fd_lock);
	if(pstream->conns[i].socket_fd == fd)
	{
	    Set_Connect_status(fd, 0);
	    closesocket(fd);
	    pstream->conns[i].socket_fd = -1;
	    pstream->conns[i].keepidle = 40;
	    pstream->conns[i].p_recv.data_len = 0;
	    pstream->conns[i].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
	}
	pthread_mutex_unlock(&pstream->conns[i].fd_lock);
    }
    if(aystream_get_status() < 0)
    {
	STATUS_CLR(STREAM_LOGIN);
	pstream->snd_report_cnt[1] = 0;
	pstream->rcv_report_cnt[1] = 0;
    }
    return 0;
}

static int  wait_data_ack(int sockfd, int usrtimeout, char *pout, int *dtlen)
{
    int 	    len, rslt;
    fd_set	    rd_set;
    struct timeval  timeout;
    char    	    buf[512];

    FD_ZERO(&rd_set);
    FD_SET(sockfd, &rd_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = usrtimeout;

    rslt = select(sockfd + 1, &rd_set, NULL, NULL, &timeout);
    if(rslt > 0)
    {
	len = recv(sockfd, (char*)&buf, sizeof(buf), 0);
	if(len == 0)
	{
	    ERRORF("ack0 recv close!\n");
	    *dtlen = 0;
	    return -1;//close
	}
	else if(len>0)
	{
	    if(len > *dtlen)
	    {
		ERRORF("ack len too big:%d>%d\n",len,*dtlen);
		*dtlen = 0;
		return -1;
	    }
	    else
	    {
		memcpy(pout, buf, len);
		*dtlen = len;
		return 0;
	    }
	}
	//else if( (errno != EAGAIN)&&(errno!= EWOULDBLOCK) )
	else
	{
	    ERRORF("ack-1 recv err->%d, %s\n",  errno, strerror(errno));
	    *dtlen = 0;
	    return -1;
	}
    }
    ERRORF("wait ack timeout: %d ms\n",usrtimeout/1000);
    return -2;
}

static int encrypt_stream_buf(int sockfd,unsigned char *buf,uint32 len)
{
    int rslt = 0;
    char key_buf[64];

    rslt = Get_exchangekey_(sockfd, key_buf);
    if ( rslt > 0)
    {
	if(anyan_device_tcp_encry_1((unsigned char *)buf, len, (unsigned char *)key_buf, rslt) != 0)
	{
	    ERRORF("anyan_devencry error,del sock:%d\n",sockfd);
	    return -1;
	}
    }
    else
    {
	ERRORF("Get exchangekey fail, ret = %d,del sock:%d\n",rslt,sockfd);
	return -1;
    }
    return 0;
}
static int decrypt_stream_buf(int sockfd,unsigned char *buf,uint32 len)
{
    int rslt = 0;
    char key_buf[64];

    rslt = Get_exchangekey_(sockfd, key_buf);
    if ( rslt > 0)
    {
	if( anyan_device_tcp_decry_1((unsigned char *)buf, len, (unsigned char *)key_buf, rslt) != 0)
	{
	    ERRORF("anyan decry_1 err\n");
	    return AY_ERROR_PRIDECRY;
	}
    }
    else
    {
	ERRORF("get exchangekey err,rslt = %d\n",rslt);  
	return AY_ERROR_EXCGKEY;
    }
    return 0;
}

static int Stream_recvdata(int sockfd, data_recv_struct *p_fd_buf)
{
    return ay_recv_stream(sockfd,p_fd_buf,1,Del_socketfd);
}

int ay_recv_stream(int sockfd, data_recv_struct *p_fd_buf,char decry,int (*onFailDeal)(int ))
{
    int	   len,ret = -1;
    size_t p_len;
    char *p_r;

    if (sizeof(p_fd_buf->data_buf) <= p_fd_buf->data_len)
    {
        ERRORF("recv buf datalen err\n");
        p_fd_buf->data_len = 0;
        onFailDeal(sockfd); //Note:we must call at the last to ensure @p_fd_buf refrence memory is valid!!
        return -1;
    }

    p_r = (char*)(p_fd_buf->data_buf + p_fd_buf->data_len);
    p_len = sizeof(p_fd_buf->data_buf) - p_fd_buf->data_len;
    len = recv(sockfd, p_r, p_len, 0);
    if (len == 0 )
    {
        ERRORF("len=0, recv err->[%d] %s\n", sockfd , strerror(errno));
        p_fd_buf->data_len = 0;
        onFailDeal(sockfd); 
        ret = 0;
    }
    else if (len > 0)
    {
        ret = len;
        DEBUGF("anyan recv data len %d\n", len);
        p_fd_buf->data_len +=  len;

UNPACK_MSG:
        if (p_fd_buf->data_len > sizeof(MSG_HEADER_C3))
        {
            MSG_HEADER_C3 *temphd = (MSG_HEADER_C3*)p_fd_buf->data_buf;
            if (temphd->size > FRAME_RECV_SIZE + 512)
            {
                ERRORF("msg header size[%d] error!\n", temphd->size);
                p_fd_buf->data_len = 0;
                onFailDeal(sockfd);
                ret = -1;
            }
            else if (temphd->size <= p_fd_buf->data_len)
            {
                if (decry && decrypt_stream_buf(sockfd, (unsigned char *)p_fd_buf->data_buf, temphd->size) < 0)
                {// only size is not encrypted
                    p_fd_buf->data_len = 0;
                    onFailDeal(sockfd);
                    ret = -1;
                }
                else if (Unpack_Msg(sockfd, p_fd_buf->data_buf, temphd->size) < 0)
                {
                    p_fd_buf->data_len = 0;
                    onFailDeal(sockfd);
                    ret = -1;
                }
                else
                {
                    p_fd_buf->data_len = p_fd_buf->data_len - temphd->size;
                    memmove(p_fd_buf->data_buf, p_fd_buf->data_buf + temphd->size, p_fd_buf->data_len);
                    goto UNPACK_MSG;
                }
            }
        }
    }
    else if ((errno != EAGAIN) && (errno!= EWOULDBLOCK))
    {
        ERRORF("{{StreamConnEvent}}DisconnectPassive|len1=%d, recv err->[%d] %s\n", len, sockfd, strerror(errno));
        p_fd_buf->data_len = 0;
        onFailDeal(sockfd);
        ret = -1;
    }
    return ret;
}

static int Stream_senddata(int sockfd[], int size, char *buf, int dtlen, char encry)
{
    int 		send_pos = 0;
    int 		sended_len = 0, i, maxfd, rslt;
    fd_set		wt_set;
    struct timeval	timeout;

    trace_update_net_stat_info(dtlen,0,0);

    FD_ZERO(&wt_set);
    maxfd = sockfd[0];
    for ( i = 0 ; i < size ; i++ )
    {
	FD_SET(sockfd[i], &wt_set);
	if ( maxfd < sockfd[i])
	{
	    maxfd = sockfd[i];
	}
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 30000;
    rslt = select(maxfd + 1, NULL, &wt_set, NULL, &timeout);
    if(rslt > 0)
    {
	send_pos = rand() % rslt;//First select a socket by random
	for ( i = 0 ; i < size; i++)
	{
	    if (FD_ISSET(sockfd[i], &wt_set))
	    {
		if (sended_len++ == send_pos)//if the socket is selected,we send
		{
		    rslt = ay_send_stream(sockfd[i],buf,dtlen,encry,Del_socketfd);
		    if(rslt > 0) trace_update_net_stat_info(0,dtlen,0);
		    return rslt;
		}
	    }
	}
    }
    return -2;
}

int ay_send_stream(int sockfd, char *buf, int dtlen, char encry,int (*onFailDeal)(int ))
{
    int send_pos = 0;
    int sended_len = 0;
    int rsnd = 0;
    fd_set		wt_set;
    struct timeval	timeout;

    if(encry && encrypt_stream_buf(sockfd,(unsigned char *)buf,dtlen)<0)
    {
	onFailDeal(sockfd);
	return -1;
    }
    while(rsnd < 100)
    {// 5ms * 100 = 500ms
	sended_len = send(sockfd, buf + send_pos, dtlen - send_pos, 0);
	if (sended_len == -1 )
	{
	    if((errno != EINTR) && (errno != EAGAIN) && (errno!= EWOULDBLOCK))
	    {
		ERRORF("snd err,%s,close [%d]\n", strerror(errno), sockfd);
		onFailDeal(sockfd);
		return -1;
	    }
	    else
	    {
		rsnd ++ ;
		FD_ZERO(&wt_set);
		FD_SET(sockfd, &wt_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 5000;
		select(sockfd + 1, NULL, &wt_set, NULL, &timeout);
	    }
	}
	else if ( sended_len < dtlen - send_pos)
	{
	    send_pos += sended_len;
	}
	else
	{
	    return dtlen;
	}
    }
    return -1;
}

static int  login_stream_server(int sockfd,st_ay_dev_info *pdevinfo)
{
    char  buf[512], recvbuf[512];
    int   len, rslt, recvlen = 0;
    MsgDeviceLoginRequest msg_login;

    msg_login.mask = 0x01;
    msg_login.did.device_id_length = pdevinfo->did.device_id_length;
    memcpy(msg_login.did.device_id, pdevinfo->did.device_id,sizeof(pdevinfo->did.device_id));

    msg_login.token.token_bin_length = pdevinfo->token.token_bin_length;
    memcpy(msg_login.token.token_bin, pdevinfo->token.token_bin,sizeof(pdevinfo->token.token_bin));

    msg_login.vs[0] = COMPILE_COMMAND;
    msg_login.vs[1] = (GCC_VERSION_1 << 12) + (GCC_VERSION_2 << 8) + (GCC_VERSION_3 << 4)+GCC_TYPE_LIB;
    msg_login.vs[2] = (YEAT << 8) + MONTH;
    msg_login.vs[3] = (DAYS << 8) + COUNT;

    msg_login.status_mask = pdevinfo->dev_status_mask;

    msg_login.dev_type 		= pdevinfo->dev_type;
    msg_login.video_type 	= ((pdevinfo->video_type<<4)&0xf0)|(pdevinfo->audio_type & 0x0f);
    msg_login.channel_num 	= pdevinfo->channelnum;
    msg_login.max_rate 		= pdevinfo->max_rate;
    msg_login.min_rate 		= pdevinfo->min_rate;

    msg_login.mask |= 0x02;
    msg_login.hi_private_addr.IP = pdevinfo->local_net_ip;
    msg_login.hi_private_addr.Port = LOCAL_PORT_VLU;

    struct timespec time_tmp= { 0,0 };
    struct timespec tick = { 0,0 };
    msg_login.mask |= 0x04;
    clock_gettime(CLOCK_REALTIME, &time_tmp);
    clock_gettime(CLOCK_MONOTONIC, &tick);
    msg_login.current_time = time_tmp.tv_sec;
    msg_login.current_tick = tick.tv_sec;// tick.tv_sec *1000 + tick.tv_nsec/1000000;

    //msg_login.mask |= 0x08;
    msg_login.mask |= 0x10;
    msg_login.uchannelcount = pdevinfo->audio_chnl;
    msg_login.uSamplerate = pdevinfo->audio_smaple_rt;
    msg_login.ubitLength = pdevinfo->audio_bit_width;

    msg_login.flag = 0x01;//new timestamp format
    if( pdevinfo->has_tfcard == 1)
    {
	msg_login.flag |= 0x08; // TF card
	DEBUGF("set tf card flag.\n");
    }

    if ( pdevinfo->dev_type != 3)
    {
	msg_login.mask |= 0x0020;
	len = pdevinfo->channelnum/8;
	if (pdevinfo->channelnum % 8) { len += 1; }
	msg_login.channel_mask_len = len;
	memcpy(msg_login.channel_mask, pdevinfo->channel_mask, sizeof(msg_login.channel_mask));
    }
    len = Pack_MsgDeviceLoginRequest(buf, &msg_login);

    Stream_senddata(&sockfd, 1, buf, len, 1);
    //aydebug_printf("login msg", buf, len);

    recvlen = sizeof(recvbuf);
    rslt = wait_data_ack(sockfd, 1000*1000, recvbuf, &recvlen);
    if( rslt == 0)
    {// ok
	//aydebug_printf("login ack ", recvbuf, recvlen);
	if(decrypt_stream_buf(sockfd,(unsigned char *)recvbuf,recvlen)<0)
	    rslt = 1;
	else
	{
	    rslt = Unpack_Msg(sockfd, recvbuf, recvlen);
	    if(rslt>0 || rslt == DEVICE_ERR_IS_WILD)
	    {
		rslt = 0;
	    }
	    else
	    {
		rslt = 1;
	    }
	}
    }
    return rslt;
}

static int  report_stream_ability(int sockfd,st_ay_dev_info *pdevinfo)
{
	char	buf[512+256];
	int 	len;
	MsgDeviceAbilityReport ability_msg;
		

	if(pdevinfo->max_stream_num_per_chn == 0)
		return 0;
	ability_msg.mask = 0x01;

	ability_msg.max_stream_num_per_chn = pdevinfo->max_stream_num_per_chn;

	len = Pack_MsgDeviceAbilityReport(buf, &ability_msg);
	if(Stream_senddata(&sockfd, 1, buf, len, 1) < 0)
	{
		Msg_Cmd_Add_queue(buf, len);
	}
	return 1; 
		
}



static int exchange_tcp_key(int tmp_fd)
{
    int  	rslt = -1, dtlen;
    char        tmp_buf[384];
    struct ExchangeKeyResponse  pKey_Resp;
    ExchangeKeyValue 		pKey;
    do
    {
	rslt = Write_MsgExchangeKeyRequest(tmp_buf, sizeof(tmp_buf), tmp_fd);
	if(rslt <= 0)
	{
	    ERRORF("ex key failed 1\n");
	    break;
	}
	if(Stream_senddata(&tmp_fd , 1, tmp_buf, rslt, 0) < 0)
	{
	    ERRORF("ex key failed 2\n");
	    break;
	}

	memset(tmp_buf, 0, sizeof(tmp_buf));
	dtlen = sizeof(tmp_buf);
	if(wait_data_ack(tmp_fd, 1000*5000, tmp_buf, &dtlen)==0)
	{
	    memset(&pKey_Resp, 0, sizeof(pKey_Resp));

	    rslt = Unpack_MsgExchangeKeyResponse(tmp_buf, dtlen, &pKey_Resp, tmp_fd);
	    if ( rslt != 0 )
	    {
		ERRORF("ex key failed 3\n");
		break;
	    }
	    rslt = GetExchengeKey(&pKey_Resp, &pKey, tmp_fd);
	    if ( rslt != 0)
	    {
		ERRORF("ex key failed 4\n");
		break;
	    }
	    return 0;
	}
    }
    while (0);
    return -1;
}

static int aystream_login_task(int arg)
{
    // 重新登录到服务器，关闭所有流的上传
    add_channel_cmd(VIDEO_CTRL, 0, 0, ay_psdk->devinfo.max_rate, CMD_FROM_SERVER);
    add_channel_cmd(VIDEO_CTRL, 0, 0, ay_psdk->devinfo.min_rate, CMD_FROM_SERVER);

    int tmp_fd = -1, token_len = 0;
    struct  in_addr saddr;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;

    token_len = pdevinfo->token.token_bin_length;
    saddr.s_addr = pdevinfo->stream_serv_ip;
    if (token_len == 0 || saddr.s_addr == 0)
    {
        DEBUGF("stream token len = %d,stream ip = %d\n", token_len, saddr.s_addr);
        return -1;
    }

    TRACEF("{{StreamConnEvent}}Connecting|start connect %d.\n", arg);
    pstream->conns[arg].status = AYE_VAL_STREAM_STATUS_CONNECTING;
    pstream->conns[arg].myaddr.port = pdevinfo->stream_serv_port;
    strcpy(pstream->conns[arg].myaddr.ipstr, inet_ntoa(saddr));
    tmp_fd = ayutil_tcp_client2(saddr.s_addr, pstream->conns[arg].myaddr.port, 5);
    if (tmp_fd < 0)
    {
        ERRORF("{{StreamConnEvent}}ConnectFail|connect stream server[%s:%d] failed!\n", 
            pstream->conns[arg].myaddr.ipstr, pstream->conns[arg].myaddr.port);
        pstream->conns[arg].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
        return -1;
    }
    pdevinfo->local_net_ip = ayutil_get_host_ip(tmp_fd);

    /* !!! connect ok ---> login ok maybe cost 10s~20s !!! */
    TRACEF("{{StreamConnEvent}}Connecting|conn %d stream server ok.\n", arg);
    if (exchange_tcp_key(tmp_fd) == 0)
    {
        if (login_stream_server(tmp_fd, pdevinfo) != 0)
        {
            ERRORF("{{StreamConnEvent}}ConnectFail|login %d fail cls skt\n", arg);
            closesocket(tmp_fd);
            Set_Connect_status(tmp_fd, 0);
            pstream->conns[arg].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
        }
        else
        {
            TRACEF("{{StreamConnEvent}}Connected|login stream server [%s:%d] ok!\n", 
                pstream->conns[arg].myaddr.ipstr, pstream->conns[arg].myaddr.port);
            
            report_stream_ability(tmp_fd, pdevinfo); // add by che to send max stream ability
            
            pthread_mutex_lock(&pstream->conns[arg].fd_lock);
            pstream->conns[arg].socket_fd  = tmp_fd;
            pstream->conns[arg].keepidle = 40;
            pstream->conns[arg].p_recv.data_len = 0;
            pstream->conns[arg].status = AYE_VAL_STREAM_STATUS_CONNECTED;
            pstream->snd_report_cnt[1] = 0; // Reset
            pstream->rcv_report_cnt[1] = 0;
            pthread_mutex_unlock(&pstream->conns[arg].fd_lock);
            sem_post(&pstream->conns[arg].conn_ok_sem);
            STATUS_SET(STREAM_LOGIN);
            pdevinfo->expt_cycle_strm = 10; // Reset stream expected cycle to min value
            return 0;
        }
    }
    else
    {
        ERRORF("{{StreamConnEvent}}ConnectFail|EXCHANGE KEY FAILED\n");
        closesocket(tmp_fd);
        Set_Connect_status(tmp_fd, 0);
        pstream->conns[arg].status = AYE_VAL_STREAM_STATUS_DISCONNECT;
    }
    return -1;
}

static int send_frame_data_to_stream(int fdarr[], int i, frm_buf_struct msg_left,Chnl_ctrl_table_struct *ptable)
{
    int len;
    char tempbuf[8*1024+512];
    MsgStreamFrameDataResponse  frm_info;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    //printf("--->send frm[%d] seq[%u] len[%d] total[%d]\n",msg_left.frm_type,msg_left.frameid,msg_left.len,msg_left.whole_len);
    if(msg_left.offset==0 && msg_left.frm_type==CH_I_FRM)
    {// update now_send_I_ts
	update_send_I_ts(ptable,msg_left.channelnum,msg_left.frm_rate,msg_left.frm_ts);
    }
    frm_info.mask = 0x01;
    frm_info.channel_index = msg_left.channelnum;
    frm_info.update_rate = msg_left.frm_rate;
    frm_info.frm_seq = msg_left.frameid;
    frm_info.length = msg_left.len;
    frm_info.offset = msg_left.offset;

    frm_info.mask |= 0x02;
    frm_info.data = msg_left.buf;

    frm_info.mask |= 0x04;
    frm_info.frm_ts = msg_left.frm_ts;
    frm_info.frm_size = msg_left.whole_len;

    frm_info.mask |= 0x08;
    frm_info.av_frm_seq = msg_left.frm_av_id;

    frm_info.mask |= 0x10;
    frm_info.frm_time = msg_left.frm_ts_sec;

    //frm_info.mask |= 0x20;
    //frm_info.device_hash[0] = pdevinfo->did.device_id[18];
    //frm_info.device_hash[1] = pdevinfo->did.device_id[19];

    if ( msg_left.frm_type == CH_I_FRM )
    {
	frm_info.flag = enm_i_frame_flag;
    }
    else if (msg_left.frm_type == CH_P_FRM)
    {
	frm_info.flag = 0;
    }
    else if(msg_left.frm_type == CH_AUDIO_FRM)
    {
	frm_info.flag = enm_audio_flag;
    }
    else if (msg_left.frm_type == CH_HIS_I_FRM)
    {
	frm_info.flag = enm_i_frame_flag;
	frm_info.flag |= 0x10;
    }
    else if (msg_left.frm_type == CH_HIS_P_FRM)
    {
	frm_info.flag = 0;
	frm_info.flag |= 0x10;
    }
    else if (msg_left.frm_type == CH_HIS_AUDIO_FRM)
    {
	frm_info.flag = enm_audio_flag;
	frm_info.flag |= 0x10;
    }
    
    pthread_mutex_lock(&sdk_get_handle(0)->stream_flag_lock);
    if((sdk_get_handle(0)->inst_flag&SDK_FLAG_UPLOAD_MD_STREAM) && (msg_left.frm_rate==pdevinfo->min_rate))
    {
	static int last_frame = 0;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
	if (ts.tv_sec > sdk_get_handle(0)->mdcloud_start && ts.tv_sec - sdk_get_handle(0)->mdcloud_start>300 && !sdk_get_handle(0)->alarm_state) // 5 mins
	{// ensure last frame is integrity,and next frame is end frame
	    if(msg_left.offset==0) last_frame = 1;
	    if(last_frame && (msg_left.len==msg_left.whole_len%FRAME_SNED_SIZE || msg_left.offset==msg_left.whole_len-FRAME_SNED_SIZE))
	    {
		last_frame = 0;
		DEBUGF("md last end frame,whole_len = %u,pack_len = %u\n",msg_left.whole_len,msg_left.len);
		sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
		add_channel_cmd(VIDEO_CTRL,1,0,pdevinfo->min_rate,CMD_FROM_MDCLOUD);

		if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
		    add_channel_cmd(AUDIO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
	    }
	}
	else
	{
	    frm_info.flag |= enm_move_detect_frame_flag;
	}
    }

	if((sdk_get_handle(0)->inst_flag&SDK_FLAG_UPLOAD_HD_STREAM) && (msg_left.frm_rate==pdevinfo->min_rate))
    {
    	if(sdk_get_handle(0)->alarm_state == 1)
    	{
    		DEBUGF("send_frame_data_to_stream enm_move_detect_frame_flag\n");
    		frm_info.flag |= enm_move_detect_frame_flag;
    	}
		else
		{
			sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_HD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
			if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
				add_channel_cmd(AUDIO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}

    if ((sdk_get_handle(0)->inst_flag&SDK_FLAG_UPLOAD_HUMAN_STREAM) && (msg_left.frm_rate == pdevinfo->min_rate))
    {
	static int last_frame = 0;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	if (ts.tv_sec > sdk_get_handle(0)->human_detect_start && ts.tv_sec - sdk_get_handle(0)->human_detect_start > 10 && !sdk_get_handle(0)->alarm_state) // 10s
	{// ensure last frame is integrity,and next frame is end frame
	    if (msg_left.offset == 0) last_frame = 1;
	    if (last_frame && (msg_left.len == msg_left.whole_len%FRAME_SNED_SIZE || msg_left.offset == msg_left.whole_len - FRAME_SNED_SIZE))
	    {
		last_frame = 0;
		DEBUGF("md last end frame,whole_len = %u,pack_len = %u\n", msg_left.whole_len, msg_left.len);
		sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_HUMAN_STREAM;
		add_channel_cmd(VIDEO_CTRL, 1, 0, pdevinfo->min_rate, CMD_FROM_HUMAN);
	    }
	}
	else
	{
	    frm_info.mask |= enm_human_detect_frame_flag;
	    frm_info.alarm_type = 1;
	    frm_info.alarm_time = sdk_get_handle(0)->alarm_time;
	}
    }
    pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);

    frm_info.mask |= 0x80;
    frm_info.media_video_type = msg_left.media_video_type;
    frm_info.media_audio_type = msg_left.media_audio_type;
    frm_info.reserved         = 0;

    len = Pack_MsgStreamFrameDataResponse(tempbuf, &frm_info);
	//>printf("--->send frm[%d] seq[%u] len[%d] total[%d] flag[%d]\n",msg_left.frm_type,msg_left.frameid,msg_left.len,msg_left.whole_len,frm_info.flag);
    return Stream_senddata(fdarr, i, tempbuf, len, 1);
}

static int report_stream_status(st_ay_dev_info *pdevinfo)
{
    char    buf[512+256];
    int     len, i;
    MsgStatusReport  status_msg;
    struct timespec now;
    static struct timespec period;
    static int    min_rate_s = 0, max_rate_s = 0, status_mask_s = 0;
    static uint8  channel_mask_s[16]={0};

    if(pdevinfo->expt_cycle_strm > 30) 
	i = 30;
    else if(pdevinfo->expt_cycle_strm < 10) 
	i = 10;
    else 
	i = pdevinfo->expt_cycle_strm;

    clock_gettime(CLOCK_MONOTONIC,&now);
    if(pdevinfo->token_update_flag || pdevinfo->devstatus_update_flag || (ayutil_cost_mseconds(period,now) >= i*1000))
    {
	period = now;
	if(pdevinfo->devstatus_update_flag) pdevinfo->devstatus_update_flag = 0;
	if(sdk_get_handle(0)->stream.snd_report_cnt[1] == 0)
	TRACEF("report status to stream..0x%x\n", pdevinfo->dev_status_mask);
	else
	DEBUGF("report status to stream..0x%x\n", pdevinfo->dev_status_mask);

	status_msg.mask = 0;

	if (status_mask_s != pdevinfo->dev_status_mask)
	{
	    status_msg.mask = 0x01;
	    status_mask_s = pdevinfo->dev_status_mask;
	    status_msg.status_mask = pdevinfo->dev_status_mask;
	}

	if (min_rate_s != pdevinfo->min_rate || max_rate_s != pdevinfo->max_rate)
	{
	    min_rate_s = pdevinfo->min_rate;
	    max_rate_s = pdevinfo->max_rate;

	    status_msg.mask |= 0x02;
	    status_msg.min_rate = pdevinfo->min_rate;
	    status_msg.max_rate = pdevinfo->max_rate;
	}

	if ( pdevinfo->token_update_flag == 1 )
	{
	    status_msg.mask |= 0x08;
	    status_msg.stream_serv_ip = pdevinfo->stream_serv_ip;
	    status_msg.stream_serv_port = pdevinfo->stream_serv_port;
	    status_msg.new_token.token_bin_length = pdevinfo->token.token_bin_length;
	    memcpy(status_msg.new_token.token_bin, pdevinfo->token.token_bin, pdevinfo->token.token_bin_length);

	    pdevinfo->token_update_flag = 0;
	}

	if(memcmp(channel_mask_s,status_msg.channel_mask,sizeof(channel_mask_s)))
	{
	    status_msg.mask |= 0x10;
	    len = pdevinfo->channelnum / 8;
	    if (pdevinfo->channelnum % 8) { len += 1; }

	    status_msg.channel_mask_len = len;
	    memcpy(status_msg.channel_mask, pdevinfo->channel_mask, sizeof(status_msg.channel_mask));
	    memcpy(channel_mask_s, pdevinfo->channel_mask, sizeof(channel_mask_s));
	}

	len = Pack_MsgStatusReport(buf, &status_msg);
	Msg_Cmd_Add_queue(buf, len);
	return 1; // send ok(later)
    }
    return 0;
}

void *aystream_send_thread(void* arg)
{
    uint32 tmpct;
    int  i, rlst,len,report_cnt = 0,wait_stream_cnt = 0;
    int fdarr[MAX_STREAM_THREAD_NUM];
    frm_buf_struct msg_left;
    char tempbuf[8*1024+512];
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    Chnl_ctrl_table_struct *ptable = ay_psdk->devobj[0].streamFilter;

    static uint32 msg_serial_num = 0;

    set_thread_name("ulk_stmsnd");
    while(!sdk_get_handle(0)->exit_flag)
    {
	if((i = Get_socketfd(fdarr,NULL)) > 0)
        {
	    if((len = Msg_Cmd_Get_queue(tempbuf,&tmpct)) > 0)
	    //if((len = Msg_Cmd_Get_queue_timedwait(tempbuf,300)) > 0)
	    {
		rlst = Stream_senddata(fdarr, i, tempbuf, len, 1);
		if(rlst >= 0)
		{
		    Msg_Cmd_Del_queue_first();
		}
	    }
	    else if(Get_frame_buf_list_first(&msg_left) > 0)
	    //if(Get_frame_buf_list_first_timedwait(&msg_left,100) == 0)
	    {
		if(is_chn_filter(ptable,msg_left.channelnum,msg_left.frm_type,msg_left.frm_rate))
		{
		    Del_frame_buf_list_first(msg_left.frameid);
		    continue;
		}
		if(msg_serial_num != msg_left.msg_serial_num)
		{
		    msg_serial_num = msg_left.msg_serial_num;
		    sdk_get_handle(0)->net_stream_upload_bytes += msg_left.len;
		}
		wait_stream_cnt = 0;
		pstream->status = AYE_VAL_STREAM_STATUS_SENDDAT;
		rlst = send_frame_data_to_stream(fdarr, i, msg_left,ptable);
		if (rlst >= 0)
		{
		    Del_frame_buf_list_first(msg_left.frameid);
		}
	    }
	    else
	    {
		if(wait_stream_cnt++ > 100) 
		{
		    wait_stream_cnt = 0;
		    pstream->status = AYE_VAL_STREAM_STATUS_WAITDAT;
		}
		//ayutil_sleep(50);
		ayutil_wait_sem(&pstream->data_ok_sem,50);
	    }
	    report_cnt = report_stream_status(&sdk_get_handle(0)->devinfo);
	    pstream->snd_report_cnt[0] += report_cnt;
	    pstream->snd_report_cnt[1] += report_cnt;
	}
	else
	{
	    pstream->status = AYE_VAL_STREAM_STATUS_WAITREQ;
	    //ayutil_sleep(200);
	    ayutil_wait_sem(&pstream->conns[0].conn_ok_sem,100);
	}
    }
    return NULL;
}

void *aystream_recv_thread(void* arg)
{
    int	 i, rslt,report_wait_ack = 0, aystream_trynext_wait = 1;
    int  fdarr[MAX_STREAM_THREAD_NUM];
    struct timespec	t1,t2;
    data_recv_struct *p_fd_buf[MAX_STREAM_THREAD_NUM]={0};
    st_ay_stream_ctrl *pstream = &sdk_get_handle(0)->stream;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    set_thread_name("ulk_stmrecv");
    clock_gettime(CLOCK_MONOTONIC,&t1);
    while(!sdk_get_handle(0)->exit_flag)
    {
	if(aystream_get_status() < 0)
	{
	    if(pdevinfo->stream_serv_ip > 0)
	    {
		clock_gettime(CLOCK_MONOTONIC,&t2);
		if(inet_addr(pstream->conns[0].myaddr.ipstr) != pdevinfo->stream_serv_ip 
			|| pstream->conns[0].myaddr.port!=pdevinfo->stream_serv_port)
		{
		    t1 = t2;
		    if(aystream_login_task(0) < 0) 
		    {
			aystream_trynext_wait = 1;
			ayutil_sleep(100);
			continue;
		    }
		}
		else if(t2.tv_sec - t1.tv_sec >= aystream_trynext_wait)
		{
		    t1 = t2;
		    if(aystream_login_task(0) < 0)
		    {
			if(aystream_trynext_wait < 420)
			    aystream_trynext_wait *= 2;
			ayutil_sleep(100);
			continue;
		    }
		}
		else
		{
		    ayutil_sleep(200);
		    continue;
		}
	    }
	    else
	    {
		ayutil_sleep(200);
		continue;
	    }
	}

	aystream_trynext_wait = 1; // seconds
	report_wait_ack = pstream->snd_report_cnt[1] - pstream->rcv_report_cnt[1];
	if(report_wait_ack > 3) // 3 * 30 = 90s
	{
	    ERRORF("{{StreamConnEvent}}DisconnectActive|status report wait_ack = %d,scc[%d],rcc[%d],scl[%d],rcl[%d]!\n",
		    report_wait_ack,pstream->snd_report_cnt[0],pstream->rcv_report_cnt[0],
		    pstream->snd_report_cnt[1],pstream->rcv_report_cnt[1]);
	    aystream_reset_conns();
	}
	else if(inet_addr(pstream->conns[0].myaddr.ipstr) != pdevinfo->stream_serv_ip 
		|| pstream->conns[0].myaddr.port!=pdevinfo->stream_serv_port)
	{
	    struct in_addr saddr;
	    saddr.s_addr = pdevinfo->stream_serv_ip;
	    ERRORF("{{StreamConnEvent}}DisconnectActive|reset stream conns,last streamip:%s,nowip:%s\n",
		    pstream->conns[0].myaddr.ipstr,inet_ntoa(saddr));
	    aystream_reset_conns();
	}
	else if((i = Get_socketfd(fdarr,p_fd_buf)) > 0)
	{
	    int		maxfd,j;
	    fd_set	rd_set,err_set;
	    struct timeval timeout;

	    maxfd = fdarr[0];
	    FD_ZERO(&rd_set);
	    FD_ZERO(&err_set);
	    for (j = 0; j < i; j++ )
	    {
		FD_SET(fdarr[j], &rd_set);
		FD_SET(fdarr[j], &err_set);
		if ( maxfd < fdarr[j])
		{
		    maxfd = fdarr[j];
		}
	    }
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 30*1000;
	    rslt = select(maxfd + 1, &rd_set, NULL, &err_set, &timeout);
	    if(rslt > 0)
	    {
		for ( j = 0 ; j < i; j++ )
		{
		    if (FD_ISSET(fdarr[j], &rd_set))
		    {
			Stream_recvdata(fdarr[j], p_fd_buf[j]);
		    }
		    else if(FD_ISSET(fdarr[j],&err_set))
		    {
			ERRORF("{{StreamConnEvent}}DisconnectActive|select exception,j = %d\n",j);
			Del_socketfd(fdarr[j]);
		    }
		}
	    }
	    else if(rslt < 0)
	    {// some exception
		ERRORF("{{StreamConnEvent}}DisconnectActive|recv select error,rslt = %d\n",rslt);
		aystream_reset_conns();
	    }
	    else
	    {// timeout
		;
	    }
	}
	else
	{
	    ayutil_sleep(100);
	}
    }
    return NULL;
}

int aystream_add_frame(st_ay_device_object *pdevobj,Stream_Event_Struct *pEvent)
{
    unsigned int i, tmp, rate_index = 0;
    int rlst = 0;
    frm_buf_struct  msg_left;
    struct timespec time_tmp = { 0,0 };
    int chn = pEvent->channelnum - 1;;
    Chnl_ctrl_table_struct *ptable = pdevobj->streamFilter;

    rate_index = ayutil_query_rate_index(pEvent->bit_rate);
    if(is_chn_filter(ptable,pEvent->channelnum,pEvent->frm_type, pEvent->bit_rate))
    {
#if 0
	if ( time(NULL) % 100 == 0)
	{
	    DEBUGF("%d(chn),%d(brate),%d(frmtype) filter\n", pEvent->channelnum, pEvent->bit_rate,pEvent->frm_type);
	}
	return -2;
#else
	return 0;
#endif
    }
    if(IS_LIVE_FRAME(pEvent->frm_type))
    {
	if(pEvent->frm_type==CH_I_FRM && ptable[chn].frm_id[rate_index].first_I_frame_required)
	{
	    ptable[chn].frm_id[rate_index].first_I_frame_required = 0;
	}
	else if(ptable[chn].frm_id[rate_index].first_I_frame_required)
	{
	    return -7;// we need I frame.
	}
    }
    else
    {// history frame 
	if(pEvent->frm_size > ay_get_frame_buf_free(&pdevobj->streamBuf))
	{
	    return -8;
	}
    }
    switch(pEvent->frm_type)
    {
	case CH_I_FRM:
	case CH_P_FRM:
	{
	    msg_left.frameid = ptable[chn].frm_id[rate_index].frm_id_all++;
	    msg_left.frm_av_id = ptable[chn].frm_id[rate_index].frm_vedio_id++;

	    if ( ptable[chn].frm_id[rate_index].time_second_ms_base == 0 )
	    {// first extract timebase
		clock_gettime(CLOCK_REALTIME, &time_tmp);

		ptable[chn].frm_id[rate_index].time_second_ms_base = time_tmp.tv_sec;
		ptable[chn].frm_id[rate_index].time_ms_ref = pEvent->frm_ts;
	    }
	    if ( pEvent->frm_ts < ptable[chn].frm_id[rate_index].last_time_stamp)
	    {
		clock_gettime(CLOCK_REALTIME, &time_tmp);

		ptable[chn].frm_id[rate_index].time_second_ms_base = time_tmp.tv_sec;
		ptable[chn].frm_id[rate_index].time_ms_ref = pEvent->frm_ts;
	    }

	    ptable[chn].frm_id[rate_index].last_time_stamp = pEvent->frm_ts;
	    msg_left.frm_ts = pEvent->frm_ts - ptable[chn].frm_id[rate_index].time_ms_ref;
	    break;
	}
	case CH_AUDIO_FRM:
	{
	    msg_left.frameid = ptable[chn].frm_id[rate_index].frm_id_all++;
	    msg_left.frm_av_id = ptable[chn].frm_id[rate_index].frm_audio_id++;
	    if ( ptable[chn].frm_id[rate_index].time_second_ms_base_audio == 0 )
	    {
		clock_gettime(CLOCK_REALTIME, &time_tmp);

		ptable[chn].frm_id[rate_index].time_second_ms_base_audio = time_tmp.tv_sec;
		ptable[chn].frm_id[rate_index].time_ms_ref_audio = pEvent->frm_ts;
	    }
	    if ( pEvent->frm_ts < ptable[chn].frm_id[rate_index].last_time_stamp_audio)
	    {
		clock_gettime(CLOCK_REALTIME, &time_tmp);

		ptable[chn].frm_id[rate_index].time_second_ms_base_audio= time_tmp.tv_sec;
		ptable[chn].frm_id[rate_index].time_ms_ref_audio= pEvent->frm_ts;
	    }

	    ptable[chn].frm_id[rate_index].last_time_stamp_audio= pEvent->frm_ts;
	    msg_left.frm_ts = pEvent->frm_ts - ptable[chn].frm_id[rate_index].time_ms_ref_audio;
	    break;
	}

	case CH_HIS_I_FRM:
	case CH_HIS_P_FRM:
	{
	    msg_left.frameid = ptable[chn].frm_id_his.frm_id_all++;
	    msg_left.frm_av_id = ptable[chn].frm_id_his.frm_vedio_id++;
	    msg_left.frm_ts = pEvent->frm_ts; // ms
	    break;
	}
	case CH_HIS_AUDIO_FRM:
	{
	    msg_left.frameid = ptable[chn].frm_id_his.frm_id_all++;
	    msg_left.frm_av_id = ptable[chn].frm_id_his.frm_audio_id++;
	    msg_left.frm_ts = pEvent->frm_ts; // ms
	    break;
	}
    }

    msg_left.frm_ts_sec = ptable[chn].frm_id[rate_index].time_second_ms_base;
    msg_left.frm_rate   = pEvent->bit_rate;
    msg_left.frm_type   = pEvent->frm_type;
    msg_left.channelnum = pEvent->channelnum;
    msg_left.whole_len  = pEvent->frm_size;
    msg_left.media_video_type = pEvent->media_video_type;
    msg_left.media_audio_type = pEvent->media_audio_type;

    if(( msg_left.frm_ts_sec == 0 )&&IS_LIVE_FRAME(pEvent->frm_type))
    {
	return -5;
    }	

    /* pack frame data and add them to buf */
    pthread_mutex_lock(&ay_psdk->frame_report_lock);
    tmp = pEvent->frm_size / FRAME_SNED_SIZE;
    for( i = 0; i < tmp; i++)
    {
	msg_left.offset = i * FRAME_SNED_SIZE;
	msg_left.len = FRAME_SNED_SIZE;
	msg_left.buf = pEvent->pdata + msg_left.offset;
	rlst = ay_add_chnl_frame(ptable,&pdevobj->streamBuf,&msg_left);
    }
    tmp = pEvent->frm_size % FRAME_SNED_SIZE;
    if (tmp > 0)
    {
	msg_left.offset = i * FRAME_SNED_SIZE;
	msg_left.len = tmp;
	msg_left.buf = pEvent->pdata + msg_left.offset;
	rlst = ay_add_chnl_frame(ptable,&pdevobj->streamBuf,&msg_left);
    }
    pthread_mutex_unlock(&ay_psdk->frame_report_lock);
    return  rlst;//·µ»Ø»º³åÇøÊ¹ÓÃÊýÁ¿
}

