#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "aylan.h"
#include "ayutil.h"
#include "ay_sdk.h"

int ay_init_client_object(st_ay_device_object *pdevobj,int clifd,int status)
{
    st_ay_client_object *pcliobj = pdevobj->pclients;
    st_ay_client_object *pnewcli = NULL;

    if(pcliobj!=NULL)
    {
		DEBUGF("only one LAN client support!\n");
		return -1; // only one LAN client permitted now
    }

    pnewcli = (st_ay_client_object*)malloc(sizeof(st_ay_client_object));
    if(pnewcli==NULL)
    {
		DEBUGF("malloc ay client object fail!\n");
		return -1;
    }
    memset(pnewcli,0,sizeof(st_ay_client_object));
    pnewcli->fd = clifd;
    pnewcli->status = status;
    pnewcli->expiretime = time(NULL)+10;
    pnewcli->pnext = NULL;

    FD_SET(clifd,&pdevobj->rdset);
    FD_SET(clifd,&pdevobj->wrset);
    if(clifd > pdevobj->maxfd)
    {
		pdevobj->maxfd = clifd;
    }

    if(pcliobj==NULL)
    {
		pdevobj->pclients = pnewcli;
    }
    else
    {
		for(;pcliobj->pnext!=NULL;pcliobj=pcliobj->pnext) ;
		pcliobj->pnext = pnewcli;
    }
    return 0;
}

int ay_lan_delete_client(int fd)
{
    if(fd > 0)
    {
		st_ay_device_object *pdevobj = &sdk_get_handle(0)->devobj[1];
		st_ay_client_object *pcliobj = pdevobj->pclients;
		st_ay_client_object *pprecli = pcliobj;

		//FIXME: reset streamfilter,because we has only one user
		disable_stream_request(pdevobj->streamFilter,CMD_FROM_CLIENT);
		//memset(pdevobj->streamFilter,0,sizeof(pdevobj->streamFilter));
	
		FD_CLR(fd,&pdevobj->rdset);
		FD_CLR(fd,&pdevobj->wrset);
		for(;pcliobj!=NULL;pcliobj=pcliobj->pnext)
		{
			if(pcliobj->fd == fd)
			{
				DEBUGF("lan delete client[%d]\n",fd);
				pcliobj->fd = -1;
				pcliobj->status = AYE_CLIOBJ_STATUS_INIT;
				if(pcliobj==pdevobj->pclients)
				{// First & Head
					pdevobj->pclients = pcliobj->pnext;
					free(pcliobj);
				}
				else
				{
					pprecli->pnext = pcliobj->pnext;
					free(pcliobj);
				}
				break;
			}
			pprecli = pcliobj;
		}
		closesocket(fd);
    }
    return 0;
}

static int send_lan_stream(int fd,frm_buf_struct *msg_left,char *tempbuf)
{
    int len;
    MsgStreamFrameDataResponse  frm_info;
    //st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    Chnl_ctrl_table_struct *ptable = sdk_get_handle(0)->devobj[1].streamFilter;

    //printf("--->lan send frm[%d] seq[%u] len[%d] total[%d]\n",msg_left->frm_type,msg_left->msg_serial_num,msg_left->len,msg_left->whole_len);
    if(msg_left->offset==0 && msg_left->frm_type==CH_I_FRM)
    {// update now_send_I_ts
	update_send_I_ts(ptable,msg_left->channelnum,msg_left->frm_rate,msg_left->frm_ts);
    }
    frm_info.mask = 0x01;
    frm_info.channel_index = msg_left->channelnum;
    frm_info.update_rate = msg_left->frm_rate;
    frm_info.frm_seq = msg_left->frameid;
    frm_info.length = msg_left->len;
    frm_info.offset = msg_left->offset;

    frm_info.mask |= 0x02;
    frm_info.data = msg_left->buf;

    frm_info.mask |= 0x04;
    frm_info.frm_ts = msg_left->frm_ts;
    frm_info.frm_size = msg_left->whole_len;

    frm_info.mask |= 0x08;
    frm_info.av_frm_seq = msg_left->frm_av_id;

    frm_info.mask |= 0x10;
    frm_info.frm_time = msg_left->frm_ts_sec;

    if ( msg_left->frm_type == CH_I_FRM )
    {
		frm_info.flag = enm_i_frame_flag;
    }
    else if (msg_left->frm_type == CH_P_FRM)
    {
		frm_info.flag = 0;
    }
    else if(msg_left->frm_type == CH_AUDIO_FRM)
    {
		frm_info.flag = enm_audio_flag;
    }
    else if (msg_left->frm_type == CH_HIS_I_FRM)
    {
		frm_info.flag = enm_i_frame_flag;
		frm_info.flag |= 0x10;
    }
    else if (msg_left->frm_type == CH_HIS_P_FRM)
    {
		frm_info.flag = 0;
		frm_info.flag |= 0x10;
    }
    else if (msg_left->frm_type == CH_HIS_AUDIO_FRM)
    {
		frm_info.flag = enm_audio_flag;
		frm_info.flag |= 0x10;
    }

    frm_info.mask |= 0x80;
    frm_info.media_video_type = msg_left->media_video_type;
    frm_info.media_audio_type = msg_left->media_audio_type;
    frm_info.reserved         = 0;

    len = Pack_MsgD2CStreamFrameDataResponse(tempbuf, &frm_info);
    return ay_send_stream(fd,tempbuf,len,0,ay_lan_delete_client);
}

int ay_lan_send_stream(st_ay_device_object *pdevobj)
{
    frm_buf_struct msg_left;
    char tempbuf[8*1024+512];

    if(ay_get_frame_buf_first(&pdevobj->streamBuf,&msg_left)>0)
    {
	st_ay_client_object *pcliobj = pdevobj->pclients;
	st_ay_client_object *ptmpobj = NULL;
	for(;pcliobj!=NULL;pcliobj=ptmpobj)
	{
	    ptmpobj=pcliobj->pnext;
	    if(pcliobj->fd>0 && 
		    !is_chn_filter(pdevobj->streamFilter,msg_left.channelnum,msg_left.frm_type,msg_left.frm_rate))
		    //!is_chn_filter(pcliobj->chnfilter,msg_left.channelnum,msg_left.frm_type,msg_left.frm_rate))
	    {
		//struct timespec ts;
		//clock_gettime(CLOCK_MONOTONIC,&ts);
		//printf("client[%d][%ld.%ld]..\n",pcliobj->fd,ts.tv_sec,ts.tv_nsec/1000);
		send_lan_stream(pcliobj->fd,&msg_left,tempbuf);
	    }
	}
	ay_del_frame_buf_first(&pdevobj->streamBuf, msg_left.frameid);
    }
    return 0;
}

static void report_device_status(st_ay_dev_info *pdevinfo)
{
    int     len, i;
    struct timespec now;
    static struct timespec period = {0,0};
    static int  min_rate_s = 0, max_rate_s = 0, status_mask_s = 0;

    //i = (pdevinfo->expt_cycle_strm > 10) ? pdevinfo->expt_cycle_strm : 10;
    i = 10; // LAN KeepLive
    clock_gettime(CLOCK_MONOTONIC,&now);
    if(ayutil_cost_mseconds(period,now) >= i*1000)
    {
	char    buf[512+256];
	MsgStatusReport  status_msg;

	period = now;

	status_msg.mask = 0;

	if ( status_mask_s != pdevinfo->dev_status_mask)
	{
	    status_mask_s = pdevinfo->dev_status_mask;
	    status_msg.mask = 0x01;
	    status_msg.status_mask = pdevinfo->dev_status_mask;
	}

	if ( min_rate_s != pdevinfo->min_rate || max_rate_s != pdevinfo->max_rate)
	{
	    min_rate_s = pdevinfo->min_rate;
	    max_rate_s = pdevinfo->max_rate;

	    status_msg.mask |= 0x02;
	    status_msg.min_rate = pdevinfo->min_rate;
	    status_msg.max_rate = pdevinfo->max_rate;
	}

	if(aystream_get_status()>0)
	{// report stream server information
	    status_msg.mask |= 0x08;
	    status_msg.stream_serv_ip = pdevinfo->stream_serv_ip;
	    status_msg.stream_serv_port = pdevinfo->stream_serv_port;
	    status_msg.new_token.token_bin_length = pdevinfo->token.token_bin_length;
	    memcpy(status_msg.new_token.token_bin, pdevinfo->token.token_bin, pdevinfo->token.token_bin_length);
	}
        status_msg.mask |= 0x10;
        len = pdevinfo->channelnum ;
        if ( len % 8 > 0)
        {
            len = (len / 8) + 1;
        }
        else
        {
            len = len / 8;
        }

        status_msg.channel_mask_len = len;
        memcpy(status_msg.channel_mask, pdevinfo->channel_mask, sizeof(status_msg.channel_mask));

	//add_msg_queue_item(&sdk_get_handle(0)->devobj[1].cmdUpQueue,buf,len,0);
	st_ay_client_object *pcliobj = sdk_get_handle(0)->devobj[1].pclients;
	st_ay_client_object *ptmpobj = NULL;
	for(;pcliobj!=NULL;pcliobj=ptmpobj)
	{
	    ptmpobj=pcliobj->pnext;
	    if(pcliobj->fd>0)
	    {
			//DEBUG("lan stream report client[%d]...\n",pcliobj->fd);
			len = Pack_MsgD2CStatusReport(buf, &status_msg);
			ay_send_stream(pcliobj->fd,buf,len,0,ay_lan_delete_client);
	    }
	}
    }
}

int ay_response_client_login(int fd,device_id_t did)
{
    char tempbuf[8*1024]={0};
    uint16 len = 0;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    MsgD2CLoginResponse msg;

    memset(&msg,0,sizeof(msg));
    msg.mask |= 0x01;

    msg.mask |= 0x02;
    msg.dev_type    	= pdevinfo->dev_type;
    msg.media_type 	= ((pdevinfo->video_type<<4)&0xf0)|(pdevinfo->audio_type & 0x0f);
    //msg.channel_num 	= pdevinfo->channelnum;
    //msg.max_rate 	= pdevinfo->max_rate;
    //msg.min_rate 	= pdevinfo->min_rate;

    msg.mask |= 0x04;
    msg.uchannelcount = pdevinfo->audio_chnl;
    msg.uSamplerate = pdevinfo->audio_smaple_rt;
    msg.ubitLength = pdevinfo->audio_bit_width;

    msg.mask |= 0x08;
    msg.status_mask = pdevinfo->dev_status_mask;

    if( pdevinfo->has_tfcard == 1)
    {
		msg.flag |= 0x04; // TF card
		DEBUGF("set tf card flag.\n");
    }

    // check device id 
    //if(strncmp((const char *)did.device_id,(const char *)pdevinfo->device_id_str,20))
    if(memcmp(did.device_id,pdevinfo->did.device_id,pdevinfo->did.device_id_length))
    {
		//DEBUG("login did[%s] incorrect!\n",did.device_id);
		msg.ret.error_code = 1001;
		strcpy((char *)msg.ret.error_description,"login did error");
		//strcpy((char *)msg.did.device_id,pdevinfo->device_id_str);
		//msg.did.device_id_length = strlen((const char *)msg.did.device_id);
		memcpy(msg.did.device_id,pdevinfo->did.device_id,pdevinfo->did.device_id_length);
		msg.did.device_id_length = pdevinfo->did.device_id_length;

		len = Pack_MsgD2CLoginResponse(tempbuf,&msg);
		if(ay_send_stream(fd,tempbuf,len,0,ay_lan_delete_client)>0)
		{// login fail,so we delete it
			ay_lan_delete_client(fd);
		}
    }
    else
    {
		memcpy(&msg.did,&did,sizeof(did));
		msg.ret.error_code = 0;
		strcpy(msg.ret.error_description,"ok");

		len = Pack_MsgD2CLoginResponse(tempbuf,&msg);
		return ay_send_stream(fd,tempbuf,len,0,ay_lan_delete_client);
    }
    return 0;
}


void *lan_stream_thread(void *arg)
{
    int servfd = -1,clifd = -1,blocks = 50;
    struct sockaddr_in clientaddr;
    socklen_t cliaddr_len = sizeof(clientaddr);
    st_ay_client_object *pcliobj = NULL,*ptmpobj;
    st_ay_device_object *pdevobj = &sdk_get_handle(0)->devobj[1];
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    if(pdevinfo->block_nums/2 > blocks)
    {
		blocks = pdevinfo->block_nums/2;
    }
    if(ay_init_device_object(pdevobj,blocks,10,10,5)<0)
    {
		DEBUGF("ay init LAN device object fail!\n");
		ay_deinit_device_object(pdevobj);
		return (void*)-1;
    }
    servfd = ayutil_tcp_server(0,LAN_STREAM_PORT,-1);
    if(servfd < 0)
    {
		DEBUGF("create lan stream server fail!\n");
		return (void*)-1;
    }

    fd_set rdset;
    int fdnum = 0;
    struct timeval tout;
    tout.tv_sec = 0;
    tout.tv_usec = 5000;

    pdevobj->handle = servfd;
    pdevobj->status = 1;
    pdevobj->maxfd = servfd;
    FD_SET(servfd,&pdevobj->rdset);
    set_thread_name("lan_thread");
    while(!sdk_get_handle(0)->exit_flag)
    {
		rdset = pdevobj->rdset;
		tout.tv_sec = 0;
		tout.tv_usec = 5000;

		fdnum = select(pdevobj->maxfd+1,&rdset,NULL,NULL,&tout);
		if(fdnum < 0 && errno!=EINTR)
		{
			DEBUGF("select err[%d]:%s\n",fdnum,strerror(errno));
		}
		if(fdnum > 0)
		{
			if(FD_ISSET(servfd,&rdset))
			{
			memset((void*)&clientaddr,0,sizeof(clientaddr));
			cliaddr_len = sizeof(clientaddr);
			clifd = accept(servfd,(struct sockaddr*)&clientaddr,&cliaddr_len);
			if(clifd > 0)
			{
				DEBUGF("accpet client[%d],ip:%s,port:%hu\n",clifd,inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
				if(ay_init_client_object(pdevobj,clifd,AYE_CLIOBJ_STATUS_LINK)<0)
				{
					closesocket(clifd);
				}
			}
			}
			else
			{
			pcliobj = pdevobj->pclients;
			for(;pcliobj!=NULL;pcliobj=ptmpobj)
			{
				ptmpobj = pcliobj->pnext;
				if(pcliobj->fd>0 && FD_ISSET(pcliobj->fd,&rdset))
				{
					if(ay_recv_stream(pcliobj->fd,&pcliobj->recvbuf,0,ay_lan_delete_client)>0)
					{
						pcliobj->expiretime = time(NULL)+30;
					}
				}
			}
			}
		}

		/* check client liveness */
		pcliobj = pdevobj->pclients;
		for(;pcliobj!=NULL;pcliobj=ptmpobj)
		{
			ptmpobj = pcliobj->pnext; // Because we may delete this object
			if(pcliobj->fd>0 && pcliobj->expiretime < time(NULL))
			{
				DEBUGF("client[%d] expired,delete it!\n",pcliobj->fd);
				ay_lan_delete_client(pcliobj->fd);
			}
		}
		// send stream to clients
		ay_lan_send_stream(pdevobj);
		report_device_status(pdevinfo);
    }

    DEBUGF("exit lan thread!\n");
    closesocket(servfd);
    ay_deinit_device_object(pdevobj);
    return NULL;
}

