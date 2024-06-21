#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "define.h"
#include "ayutil.h"
#include "ayinit.h"
#include "ayqueue.h"
#include "ayentry.h"
#include "aydes.h"
#include "aystream.h"
#include "ay_sdk.h"

#include "watchdog.h"
#include "http_base64.h"
#include "threadpool.h"
#include "protocol_query_device.h"

#include "aydevice2.h"
#include "version.h"

#ifdef WIN32
#define MSG_NOSIGNAL 0
#endif

static void encode_query_device_rsp(MsgDeviceStatusQueryResponse* rsp,st_ay_dev_info *pdevinfo)
{
    const char* dev_type_str[]=
    {
		"unk",
		"dvr",
		"nvr",
		"ipc",
		"box"
    };

    uint32 devtype;

    memset(rsp, 0x0, sizeof(MsgDeviceStatusQueryResponse));

    rsp->mask = 0x01;
    rsp->ip = ayutil_get_local_ip("eth0","ra0");
    rsp->port = LAN_STREAM_PORT;
    strcpy(rsp->factory, pdevinfo->Sn_info.Factory);
    strcpy(rsp->model, pdevinfo->Sn_info.Model);
    devtype = pdevinfo->dev_type;
    if(devtype > 4)
    {
		devtype = 0;
    }
    strcpy(rsp->device_type, dev_type_str[devtype]);
    rsp->channel_num = pdevinfo->channelnum;
    rsp->use_type = ((uint32)pdevinfo->use_type) & 0x000000ff;

    rsp->mask |= 0x02;
    strcpy(rsp->orig_id, pdevinfo->Sn_info.SN);
    rsp->factory_id = pdevinfo->Sn_info.OEMID;
    strcpy(rsp->factory_abbreviation, pdevinfo->Sn_info.OEM_name);

    rsp->mask |= 0x04;
    strcpy(rsp->device_id, (char*)pdevinfo->device_id_str);

    rsp->mask |= 0x08;
    rsp->ver[0] = COMPILE_COMMAND;
    rsp->ver[1] = (GCC_VERSION_1 << 12) + (GCC_VERSION_2 << 8) + (GCC_VERSION_3 << 4) + GCC_TYPE_LIB;
    rsp->ver[2] = (YEAT << 8) + MONTH;
    rsp->ver[3] = (DAYS << 8) + COUNT;

    rsp->mask |= 0x10;
    rsp->lan_listen_port = LAN_STREAM_PORT;
    rsp->sdkrel = Ulu_SDK_Get_Version();

    /*DEBUG("query_device-->mask=%x, factory=%s,model=%s,device_type=%s, device_id=%s\n",
      rsp->mask, rsp->factory, rsp->model, rsp->device_type, rsp->device_id);*/
}
static int send_query_device_status_rsp(int fd, struct sockaddr_in* addr)
{
    MsgDeviceStatusQueryResponse rsp;
    unsigned char msgbuf[1024];
    unsigned char cipher_buf[1024];
    int msglen, cipher_len;
    int rc = 0;
    unsigned short *len_pos;
    unsigned int rmtip = addr->sin_addr.s_addr;
    unsigned int mylip = ayutil_get_local_ip("ra0","wlan0");
    unsigned int gap1 = 0,gap2 = 0;

    encode_query_device_rsp(&rsp,&sdk_get_handle(0)->devinfo);
    if(mylip > 0 && mylip!=rsp.ip)
    {
		gap1 = htonl(mylip) > htonl(rmtip)?htonl(mylip) - htonl(rmtip):htonl(rmtip) - htonl(mylip);
		gap2 = htonl(rsp.ip) > htonl(rmtip)?htonl(rsp.ip) - htonl(rmtip):htonl(rmtip) - htonl(rsp.ip);
		//DEBUG("remoteip = %s,rmtip = %u,raip = %u,ethip = %u,gap1 = %u,gap2 = %u\n",
		//	inet_ntoa(addr->sin_addr),htonl(rmtip),htonl(mylip),htonl(rsp.ip),gap1,gap2);
		if(gap1 < gap2)
		{
			DEBUGF("use ra0 ip = %u\n",htonl(mylip));
			rsp.ip = mylip;
		}
    }

    msglen = Pack_MsgDeviceStatusQueryResponse((char*)msgbuf, &rsp);

    cipher_len = sizeof(cipher_buf);

    if( DES_Encrypt(msgbuf+2, msglen-2, (unsigned char*)_ANYAN_DEFAULT_KEY, 8, cipher_buf+2, cipher_len-2, &cipher_len) < 0)
    {
		DEBUGF("send query device response to [%s:%d] fail, encypt error!",inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
		return -1;
    }

    len_pos = (unsigned short *)&cipher_buf[0];
    cipher_len += 2;
    *len_pos = (cipher_len|0x0800);

    rc = sendto(fd, cipher_buf, cipher_len, 0, (struct sockaddr*)(addr), sizeof(struct sockaddr_in));

    //DEBUG("send query device response to [%s:%d], msg len:%d.", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), rc);

    return rc;
}

enum
{
    EN_MODE_CREATE_SOCK,
    EN_MODE_REVCE_DATA,
    EN_MODE_SOCK_ERROR,
};

void* device_discovery_thread(void *arg)
{
    int dev_discovery_mode = EN_MODE_CREATE_SOCK;
    int sockfd = -1, addrlen;
    int iRet;
    struct sockaddr_in localaddr;
    struct sockaddr_in rmtaddr;
    char recvdata[512];

    set_thread_name("ulk_dc");

    addrlen = sizeof(struct sockaddr_in);
    memset(&localaddr,0, sizeof(struct sockaddr_in));
    memset(&rmtaddr,0, sizeof(struct sockaddr_in));

    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localaddr.sin_port = htons(30061);

    while(!sdk_get_handle(0)->exit_flag)
    {
	switch(dev_discovery_mode)
	{
	    case EN_MODE_CREATE_SOCK:
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
		    ERRORF("device_discovery_thread()-->socket error:%s\n",strerror(errno));
		    break;
		}
		//���ø��׽���Ϊ�㲥����
		const int opt = 1;
		dev_discovery_mode = EN_MODE_SOCK_ERROR;
		iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
		if(iRet == -1)
		{
		    ERRORF("device_discovery_thread()-->set socketopt[SO_BROADCAST] error\n");
		    closesocket(sockfd);
		    sockfd = -1;
		    break;
		}

		struct timeval tout;
		tout.tv_sec = 3;
		tout.tv_usec = 0;
		setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,(void*)&tout,sizeof(tout));
		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(void*)&tout,sizeof(tout));

		//���Ӷ˿ڸ���,���������˿�bindʧ��
		iRet = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
		if(iRet == -1)
		{
		    ERRORF("device_discovery_thread()-->set socketopt[SO_REUSEADDR] error\n");
		    closesocket(sockfd);
		    sockfd = -1;
		    break;
		}

		iRet = bind(sockfd,(struct sockaddr *)&(localaddr), sizeof(struct sockaddr_in));
		if(iRet == -1)
		{
		    ERRORF("device_discovery_thread()-->bind error\n");
		    closesocket(sockfd);
		    sockfd = -1;
		    break;
		}
		dev_discovery_mode = EN_MODE_REVCE_DATA;
		break;
	    case EN_MODE_REVCE_DATA:
		memset(recvdata,0,sizeof(recvdata));
		iRet = recvfrom(sockfd, recvdata, sizeof(recvdata), 0, (struct sockaddr*)&rmtaddr,(socklen_t*)&addrlen);
		if(iRet < 0)
		{
		    if((errno !=EAGAIN) && (errno!=EWOULDBLOCK))
		    {
			dev_discovery_mode = EN_MODE_SOCK_ERROR;
			ERRORF("device_discovery_thread()-->recv error: %d\n", iRet);
		    }
		}
		else
		{// sizeof is unsigned int type,so ensure iRet >= 0 when compare !!!
		    if(iRet < (sizeof(MSG_HEADER_C3) + sizeof(MsgDeviceStatusQuery)))
			; //DEBUG("device_discovery_thread()-->receive msg: %d!\n",iRet);
		    else if(Unpack_MsgDeviceStatusQuery(recvdata) < 0)
		    {
			; //DEBUG("device_discovery_thread()-->receive incorrect msg!\n");
		    }
		    else
		    {
			send_query_device_status_rsp(sockfd, &rmtaddr);
		    }
		}
		break;

	    case EN_MODE_SOCK_ERROR:
	    default:
		if(sockfd != -1)
		{
		    closesocket(sockfd);
		    sockfd = -1;
		}
		dev_discovery_mode = EN_MODE_CREATE_SOCK;
		break;
	}
	sleep(1);
    }
    if(sockfd < 0)
    {
	closesocket(sockfd);
	sockfd = -1;
    }

    return NULL;
}

uint32 get_device_flag(st_ay_dev_info *pdevinfo)
{
    static char  first  = 0;
    uint32 flag = 0;
    time_t	now;
    struct  tm  *timenow, curtm;
    time(&now);
    timenow = localtime_r(&now,&curtm);

    //��һ������ʱУʱ
    if ( pdevinfo->sync_time_flag == 0 && first == 0 )
    {
		first = 1;
		flag |= 0x01;
    }
    //12 СʱУʱ..
    if(timenow->tm_hour == 12 || timenow->tm_hour == 0 )//�����12�����24��
    {
		if ( timenow->tm_min <= 2)//���ӷ����ж�,����1Сʱ�ڶ�ҪУʱ
		{
			if (pdevinfo->sync_time_flag == 0 )//�������Զ�Уʱ
			{
				flag |= 0x01;
			}
		}
    }

    if ( pdevinfo->ptz_flag == 1)
    {
		flag |= 0x40;
		flag |= 0x80;
    }else if( pdevinfo->ptz_flag == 2){
		flag |= 0x40;
		flag |= 0x80;
		flag |= 0x100;
    }else if( pdevinfo->ptz_flag == 3){
		flag |= 0x40;
		flag |= 0x80;
		flag |= 0x800; // Preset
    }else if( pdevinfo->ptz_flag == 4){
		//flag |= 0x40; // Pan
		//flag |= 0x80; // Tilt
		//flag |= 0x100; // Zoom
		//flag |= 0x800; // Preset
    }
    if ( pdevinfo->mic_flag == 1)
    {
		flag |= 0x10;
    }
    if ( pdevinfo->can_rec_voice == 1)
    {
		flag |= 0x20;
    }

    if ( pdevinfo->hard_disk == 1)
    {
		flag |= 0x200;
    }

    flag |= 0x400;

    return flag;
}

static int get_entry_loginInfo(st_ay_dev_info *pdevinfo)
{
    int 			iret = -1;
    entry_token			entry_token_buf;
    entry_request_param *pParam, entry_rq_param;
    pParam = &entry_rq_param;

    pParam->ver[0] = COMPILE_COMMAND;
    pParam->ver[1] = (GCC_VERSION_1 << 12) + (GCC_VERSION_2 << 8) + (GCC_VERSION_3 << 4)+GCC_TYPE_LIB;
    pParam->ver[2] = (YEAT<< 8) + MONTH;
    pParam->ver[3] = (DAYS<< 8) + COUNT;

    pParam->disk       = pdevinfo->hard_disk;
    pParam->audio      = pdevinfo->mic_flag;
    pParam->recv_audio = pdevinfo->can_rec_voice;
    if(pdevinfo->ptz_flag == 0)
    {
		pParam->spin = 0;
		pParam->tilt_spin = 0;
		pParam->zoom = 0;
		pParam->preset = 0;
    }
    else if(pdevinfo->ptz_flag == 1)
    {
		pParam->spin = 1;
		pParam->tilt_spin = 1;
		pParam->zoom = 0;
		pParam->preset = 0;
    }
    else if(pdevinfo->ptz_flag == 2)
    {
		pParam->spin = 1;
		pParam->tilt_spin = 1;
		pParam->zoom = 1;
		pParam->preset = 0;
    }
    else if(pdevinfo->ptz_flag == 3)
    {
		pParam->spin = 1;
		pParam->tilt_spin = 1;
		pParam->zoom = 0;
		pParam->preset = 1;
    }
    else if(pdevinfo->ptz_flag == 4)
    {
		pParam->spin = 1;
		pParam->tilt_spin = 1;
		pParam->zoom = 1;
		pParam->preset = 1;
    }
    strcpy(pParam->device_id, (char*)pdevinfo->device_id_str);

    pParam->rate_count  = 2;
    pParam->ratelist[0] = pdevinfo->min_rate;
    pParam->ratelist[1] = pdevinfo->max_rate;

#if AYDEVICE2_ENABLE
    iret = aydevice2_get_token((const char*)pdevinfo->device_id_str,&entry_token_buf);
#else
    iret = ayentry_get_token(&entry_token_buf, pParam);
#endif
    if(entry_token_buf.icode != 0 || iret == -1)
    {
		ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get entry err code = %d,ret=%d\n", entry_token_buf.icode, iret);
		return -1;
    }

    int rslt, len;
    rslt = 256;
    len = ZBase64Decode(entry_token_buf.data.token, strlen(entry_token_buf.data.token),
	    (uint8*)pdevinfo->entry_token_b64.token_bin, &rslt);
    strcpy(pdevinfo->entry_token_base64,entry_token_buf.data.token);
    pdevinfo->entry_token_b64.token_bin_length = len;

    TRACEF("{{DeviceCommEvent}}GetToken_OK|get entry token ok!\n");
    return 0;
}

static int recv_regist_msg(int sockfd,int use_short_tcp,int tout,st_ay_entry_ctrl *pentries)
{
    int rslt = 0, ret = 0,len = 0,i = 0;
    struct timeval timeout;
    fd_set rd_set;
    socklen_t addr_len;
    struct sockaddr_in 	addr;
    char tempbuf[2048] = {0};
    int recv_streaminfo_flag = 0;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    st_ay_entry *pentry = NULL;

    while(1)
    {
		timeout.tv_sec = tout;
		timeout.tv_usec = 0;
		FD_ZERO(&rd_set);
		FD_SET(sockfd,&rd_set);
		addr_len = sizeof(addr);

		ret=select(sockfd+1, &rd_set, NULL, NULL, &timeout);
		if (ret == -1)
		{
			DEBUGF("UDP/TCP select exception err!\n");
			return 0;
		}else if (ret == 0)
		{
			break;  //connect time out
		}
		if(FD_ISSET(sockfd, &rd_set)){

			len = recvfrom(sockfd, (char*)tempbuf, sizeof(tempbuf), 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
			//when closed server, recvfrom will return 0;
			if(len <= 0)
			{
				DEBUGF("%s recvfrom ret = %d,exception happens!\n", use_short_tcp?"TCP":"UDP",len);
				break;
			}

			DEBUGF("%s ack ip=%s\n", use_short_tcp?"TCP":"UDP",inet_ntoa(addr.sin_addr));

			for(i=0;i<pentries->num;i++)
			{
				pentry = &pentries->addr[i];
				if((addr.sin_addr.s_addr == pentry->ip) || (sockfd==pentry->tsock && use_short_tcp))
				{
					if(!recv_streaminfo_flag)
					{
						// just parse and handle first response with stream server information
						struct in_addr taddr;
						taddr.s_addr = pentry->ip;
						strcpy(pdevinfo->now_ack_tracker_addr,inet_ntoa(taddr));
						if(!use_short_tcp)
							anyan_device_udp_decry_1(tempbuf,len);
						ret = Unpack_Msg(sockfd, tempbuf, len);
						if(ret == AY_ERROR_TOKEN)
						{
							ERRORF("token has error,remove it!\n");
							get_entry_loginInfo(pdevinfo);
							recv_streaminfo_flag = 1;
						}
						else if(ret == AY_ERROR_STREAM)
						{
							recv_streaminfo_flag = 1;	//stream token received
						}
					}
					pentry->rtcnt = 0;
					pentry->wtout = pdevinfo->expected_cycle;
					pentry->flag = 0x00;// UDP mode
					pentry->state = 0;
					rslt = 1;
				}
			}
		}else
			break;
    }

    for(i=0;i<pentries->num;i++)
    {
		pentry = &pentries->addr[i];
		if(use_short_tcp && pentry->tsock!=sockfd)
			continue;
		if(pentry->state==1)
		{// recv timeout
			if(pentry->flag==0x01)
				pentry->flag = 0x00;
			pentry->rtcnt ++;

			if(pentry->rtcnt == 1)
				pentry->wtout = 10 + rand() % 5;
			else if(pentry->rtcnt == 2)
				pentry->wtout = 20 + rand() % 10;
			else if(pentry->rtcnt >= 3)
			{
				pentry->wtout = 40 + rand() % 20;
				pentry->rtcnt = 0;
				pentry->flag = 0x01; // TCP mode
			}
			pentry->state = 0;
		}
    }
    return rslt;
}

static int process_regist_msg(int sockfd, st_ay_entry_ctrl *pentries)
{
    int i, len;
    int	addr_len;
    int strmconn_stat;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    if(sockfd < 0 || pentries==NULL)
	return -1;

    MsgDeviceRegisterRequest  regmsg;
    struct sockaddr_in 	addr;
    char tempbuf[2048] = {0};
    char ipstr[32] = {0};
    int rslt = 0,recv_cnt = 0;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC,&now);
    for(i = 0; i < pentries->num; i++)
    {// send state
		if(pentries->addr[i].state != 0)
		{
			recv_cnt ++;
			continue;
		}
		if(ayutil_cost_mseconds(pentries->addr[i].last_sndt,now) < pentries->addr[i].wtout*1000)
			continue;

		addr.sin_family = AF_INET;
		addr.sin_port = htons(pentries->addr[i].port);
		addr.sin_addr.s_addr = pentries->addr[i].ip;
		strcpy(ipstr,inet_ntoa(addr.sin_addr));

		pentries->addr[i].last_sndt = now;
		if(pentries->addr[i].wtout == 0)
		TRACEF("{{TrackerConnEvent}}%s|UDP_OK|send tout = %d\n",ipstr,pentries->addr[i].wtout);
		else
		DEBUGF("{{TrackerConnEvent}}%s|UDP_OK|send tout = %d\n",ipstr,pentries->addr[i].wtout);

		regmsg.mask = 0x0001;
		regmsg.flag = get_device_flag(pdevinfo);

		regmsg.did.device_id_length = pdevinfo->did.device_id_length;
		memcpy(regmsg.did.device_id, pdevinfo->did.device_id,sizeof(pdevinfo->did.device_id));

		regmsg.status_mask = pdevinfo->dev_status_mask;
		STATUS_CLR(MOVE_DETECT_ALARM);
		STATUS_CLR(VIDEO_LOST_STATUS);
		STATUS_CLR(SHELTER_ALARM);

		DEBUGF("stream server token len = %d\n",pdevinfo->token.token_bin_length);
		if (pdevinfo->token.token_bin_length > 0)
		{
			regmsg.mask |= 0x0002;
			regmsg.stream_serv_ip = pdevinfo->stream_serv_ip;
			regmsg.stream_serv_port = pdevinfo->stream_serv_port;
			DEBUGF("strm serv = %u:%u\n",regmsg.stream_serv_ip,regmsg.stream_serv_port);
			strmconn_stat = aystream_get_status();
			if ( strmconn_stat  == -1)//connect fail
			{
				regmsg.flag |= 0x0004;
			}else if ( strmconn_stat == -2)//connecting
			{
				regmsg.flag |= 0x0002;
			}

			regmsg.token.token_bin_length = pdevinfo->token.token_bin_length;
			memcpy(regmsg.token.token_bin , pdevinfo->token.token_bin, sizeof(regmsg.token.token_bin));
		}

		regmsg.mask |= 0x0004;
		regmsg.vs[0] = COMPILE_COMMAND;
		regmsg.vs[1] = (GCC_VERSION_1 << 12) + (GCC_VERSION_2 << 8) + (GCC_VERSION_3 << 4)+GCC_TYPE_LIB;
		regmsg.vs[2] = (YEAT << 8) + MONTH;
		regmsg.vs[3] = (DAYS << 8) + COUNT;

		regmsg.dev_type = pdevinfo->dev_type;
		regmsg.video_type = (pdevinfo->audio_type&0x0f)|((pdevinfo->video_type<<4)&0xf0);
		regmsg.channel_num = pdevinfo->channelnum;
		regmsg.max_rate = pdevinfo->max_rate;
		regmsg.min_rate = pdevinfo->min_rate;

		regmsg.mask |= 0x0008;
		regmsg.hi_private_addr.IP = pdevinfo->local_net_ip;
		regmsg.hi_private_addr.Port = LOCAL_PORT_VLU;

		DEBUGF("entry server token len = %d\n",pdevinfo->entry_token_b64.token_bin_length);
		if(pdevinfo->entry_token_b64.token_bin_length > 0)
		{
			regmsg.mask |= 0x0010;
			regmsg.reg_token.token_bin_length = pdevinfo->entry_token_b64.token_bin_length;
			memcpy(regmsg.reg_token.token_bin, pdevinfo->entry_token_b64.token_bin, regmsg.reg_token.token_bin_length);
		}

		if ( pdevinfo->video_width > 0)
		{
			regmsg.mask |= 0x0040;
			regmsg.video_height = pdevinfo->video_height;
			regmsg.video_width = pdevinfo->video_width;
		}

		if ( pdevinfo->dev_type != 3)
		{
			regmsg.mask |= 0x0080;
			len = pdevinfo->channelnum / 8;
			if (pdevinfo->channelnum % 8)
			{
				len += 1;
			}
			regmsg.channel_mask_len = len;
			memcpy(regmsg.channel_mask, pdevinfo->channel_mask, sizeof(regmsg.channel_mask));
		}
		if ( pdevinfo->rtsp_url[0] != 0)
		{
			regmsg.mask |= 0x0100;
			strncpy(regmsg.rtsp_url,pdevinfo->rtsp_url,127);
		}

		regmsg.mask |= 0x0020;
		regmsg.entry_ip = pentries->addr[i].ip;

		len = Pack_MsgDeviceRegisterRequest(tempbuf, &regmsg);
		//aydebug_printf("---->>", tempbuf, len);

		if(pentries->addr[i].flag&0x01)
		{// tcp mode
			TRACEF("{{TrackerConnEvent}}%s|UDP_FAIL|try tcp mode!\n",ipstr);
			int tfd = ayutil_tcp_client2(pentries->addr[i].ip,pentries->addr[i].port,1);
			if(tfd > 0)
			{
				addr_len = send(tfd,tempbuf,len,MSG_NOSIGNAL);
				if(addr_len < 0)
				{
					ERRORF("{{TrackerConnEvent}}%s|TCP_FAIL|send fail:%s\n", ipstr, strerror(errno));
					closesocket(tfd);
				}
				else
				{
					DEBUGF("%s:%d> tcp send data len = %d\n", ipstr, pentries->addr[i].port, addr_len);
					pentries->addr[i].state = 1;// recv state
					pentries->addr[i].tsock = tfd;
					rslt = recv_regist_msg(tfd,1,5,pentries);
					if(rslt) TRACEF("{{TrackerConnEvent}}%s|TCP_OK|recv ok!\n",ipstr);
					closesocket(tfd);
				}
			}
			pentries->addr[i].flag = 0x00; // udp mode
			pentries->addr[i].tsock = -1;
			pentries->addr[i].rtcnt = 0;
		}
		else
		{// udp mode
			anyan_device_udp_encry_1(tempbuf, len);
			addr_len = sendto(sockfd, tempbuf, len, 0, (struct sockaddr*)(&addr), sizeof(addr));
			if(addr_len < 0)
			{
				ERRORF("{{TrackerConnEvent}}%s|UDP_FAIL|send fail:%s\n", ipstr, strerror(errno));
			}
			else
			{
				pentries->addr[i].state = 1;// recv state
			}
		}

		pdevinfo->entry_timeout++;
		ayutil_sleep(5);

		if(pentries->addr[i].state==1)
		{
			recv_cnt ++;
		}
    }

    if(recv_cnt > 0)
    {
	//DEBUG("[%ld]regist msg recv....\n",time(NULL));
		rslt = recv_regist_msg(sockfd,0,5,pentries);
    }
    if(rslt)
		pdevinfo->entry_timeout = 0;
	return rslt;
}

static int get_ipconfig(st_ay_entry_ctrl *list)
{
#if 0
    list->num = 2;

    list->addr[0].port = 9000;
    list->addr[0].ip= inet_addr("192.168.18.183");
    list->addr[0].flag = 0;
    list->addr[1].port = 9000;
    list->addr[1].ip= inet_addr("192.168.18.184");
    list->addr[1].flag = 0;
    return 0;
#else
    int  nRet, i;
    entry_server_list serverlist;
    nRet = ayentry_get_entryserver(&serverlist);
    if ( nRet != -1)
    {
		list->num = serverlist.num;
		for (i = 0; i < serverlist.num; i++)
		{
			list->addr[i].port = serverlist.list[i].port;
			list->addr[i].ip= inet_addr(serverlist.list[i].IP);
			list->addr[i].flag = 0;
			list->addr[i].state = 0;
			list->addr[i].rtcnt = 0;
			list->addr[i].wtout = 0;
			clock_gettime(CLOCK_MONOTONIC,&list->addr[i].last_sndt);
		}
		TRACEF("{{DeviceCommEvent}}GetTrackerConfig_OK|serverlist num = %d\n", serverlist.num);
		return  0;
    }
    ERRORF("{{DeviceCommEvent}}GetTrackerConfig_FAIL|get entry servers fail!\n");
    return -1;
#endif
}

static int get_device_id(st_ay_dev_info *pdevinfo)
{
    int ret, baselen;
    int rslt = 21;
    char szdevice_id[21] = {0};

    ret = ayutil_read_file(pdevinfo->rw_path, "SN_ulk", szdevice_id, 20);
    if(ret < 0)
    {
		if(strlen(pdevinfo->Sn_info.SN) == 20)
		{
			strncpy(szdevice_id,pdevinfo->Sn_info.SN, 20);
			ret = 0;
		}
		else
		{
#if AYDEVICE2_ENABLE
			ret = aydevice2_get_20sn(&pdevinfo->Sn_info,szdevice_id);
#else
			ret = ayentry_register_device(szdevice_id, &pdevinfo->Sn_info);
#endif
		}
    }

    if ( ret == 0 )
    {
		snprintf(pdevinfo->device_id_str,sizeof(pdevinfo->device_id_str),"%s",szdevice_id);
		TRACEF("{{DeviceCommEvent}}Register_OK|register device id: %s\n", szdevice_id);
		ayutil_save_file(pdevinfo->rw_path,"SN_ulk",szdevice_id,strlen(szdevice_id));
#if AYDEVICE2_ENABLE
		ret = aydevice2_login(szdevice_id,pdevinfo->min_rate,pdevinfo->max_rate);
		if(ret < 0)
		{
			ERRORF(" device2 login fail: %s\n", szdevice_id);
			return -1;
		}
#endif

		STATUS_SET(ON_LINE);
		pdevinfo->local_net_ip = ayutil_get_local_ip("eth0", "ra0");
		baselen = ZBase64Decode(szdevice_id, strlen(szdevice_id), (unsigned char*)pdevinfo->did.device_id, &rslt);
		if ( baselen <= 21 )
		{
			pdevinfo->did.device_id_length = (uint8)baselen;
		}
		return 0;
    }
    ERRORF("{{DeviceCommEvent}}Register_FAIL|register device id fail.\n");
    return -1;
}

static void device_online()
{
    CMD_PARAM_STRUCT err;
    err.channel = 0;
    err.cmd_id = EXT_DEVICE_ONLINE;
    Msg_Cmd_Add_down_queue((char*)&err, sizeof(CMD_PARAM_STRUCT));
}

void *Regist_thread_C3(void *args)
{
    int	entry_skfd = -1;
    int rslt = 0;
    struct timespec t0,t1,t2;
    st_ay_entry_ctrl  ip_list;
    int err_cnt = 0;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    int code = pdevinfo->resp_json_code;

    set_thread_name("ulk_reg");
    clock_gettime(CLOCK_MONOTONIC,&t1);
    while(!sdk_get_handle(0)->exit_flag)
    {
		switch(rslt)
		{
		case 0:
			if(get_device_id(pdevinfo)<0)
			{
				call_back_error_info("register fail\n");
				break;
			}else
				rslt = 1;
		case 1:
			if(get_ipconfig(&ip_list)<0)
			{
				call_back_error_info("get entry ip fail\n");
				break;
			}else
				rslt = 2;
		case 2:
			if(get_entry_loginInfo(pdevinfo)<0)
			{
				call_back_error_info("get login token err\n");
				break;
			}else
				rslt = 3;
		case 3:
			entry_skfd = ayutil_init_udp(LOCAL_PORT_VLU);
			if(entry_skfd > 0)
				rslt = 4;
			break;
		case 5: // idle wait status
			if(err_cnt == (1000 + rand()%1000))
			{
				rslt = err_cnt = code = 0;
			}
			break;
		}
		if(rslt == 4)
			break;

		if(pdevinfo->resp_json_code!=0 && (code==pdevinfo->resp_json_code))
		{
			if(err_cnt ++ == 10)
				rslt = 5;
		}
		else
		{
			err_cnt = 0;
			code = pdevinfo->resp_json_code;
		}
		ulumon_msg_check(2);
		sleep(5);
    }

	device_online();  //设备已上线

    clock_gettime(CLOCK_MONOTONIC,&t0);
    while(!sdk_get_handle(0)->exit_flag)
    {
		if(ip_list.num <= 0)
		{
			get_ipconfig(&ip_list);
		}

		if (ip_list.num>0 && pdevinfo->entry_timeout/ip_list.num>20)
		{
			ERRORF("20 round recv nothing response\n");
			pdevinfo->entry_timeout = 0;
			if(entry_skfd > 0)
			{
				closesocket(entry_skfd);
				entry_skfd = -1;
			}
			get_ipconfig(&ip_list);
			entry_skfd = ayutil_init_udp(LOCAL_PORT_VLU);
		}

		rslt = process_regist_msg(entry_skfd, &ip_list);

		if (rslt == 1 && pdevinfo->wifi_online_broad == 1)
		{
			pdevinfo->wifi_online_broad = 0;
		}
		sleep(3);

		clock_gettime(CLOCK_MONOTONIC,&t2);
		if(t2.tv_sec - t0.tv_sec >= (12+rand()%12)*3600)
		{/* update entry server list every [12,24] hours */
			t0 = t2;
			pdevinfo->entry_timeout = 0;
			get_ipconfig(&ip_list);
			Clean_All_Chnl_Second_Base(sdk_get_handle(0)->devobj[0].streamFilter);
			Clean_All_Chnl_Second_Base(sdk_get_handle(0)->devobj[1].streamFilter);
			TRACEF("update entry list.\n");
		}
    }

    if(entry_skfd > 0)
    {
		closesocket(entry_skfd);
		entry_skfd = -1;
    }
    return NULL;
}

void* Interact_CallBack_Thread(void* arg)
{
    CMD_PARAM_STRUCT    tmp_cmd;
    int     tmp_len,domainip_interval_secs = 10,cnt = 0;
    struct timespec	t1,t2;
    Interact_CallBack   p_callbk_fun = sdk_get_handle(0)->cbfuncs.pCmdCbfc;
    Interact_CallBack   p_oem_fun = sdk_get_handle(0)->cbfuncs.pOemCbfc;

	DEBUGF("Interact_CallBack_Thread+++\n");
    set_thread_name("ulk_cmd");
    clock_gettime(CLOCK_MONOTONIC, &t1);
    while(!sdk_get_handle(0)->exit_flag)
    {
	/* avoid set callback function after sdk-init that cause unset */
		p_callbk_fun = sdk_get_handle(0)->cbfuncs.pCmdCbfc;
		p_oem_fun = sdk_get_handle(0)->cbfuncs.pOemCbfc;
		tmp_len = Msg_Cmd_Get_down_queue((char*)&tmp_cmd);
		if ( tmp_len > 0 )
		{
			if ( p_callbk_fun != NULL)
			{
				if(p_oem_fun) (*p_oem_fun)(&tmp_cmd);
				(*p_callbk_fun)(&tmp_cmd);
				Msg_Cmd_Del_queue_down_first();
			}
			else
			{
				ERRORF("cmd callbk func null\n");
				ayutil_sleep(1000);
			}
		}
		else
		{
			ayutil_sleep(100);
		}

		clock_gettime(CLOCK_MONOTONIC,&t2);
		if(t2.tv_sec - t1.tv_sec >= 5)
		{
			if(Get_All_chnl_status(sdk_get_handle(0)->devobj[0].streamFilter))
			{// only calculate WAN stream net-info
				Computer_Net_UploadSpeed(t2.tv_sec-t1.tv_sec);
			}
			cnt++;
			if(5*cnt >= domainip_interval_secs)
			{
				int ret1 = 0,ret2 = 0,ret3 = 0;
				cnt = 0;
				ret1 = aydevice2_update_domainip(AYDEVICE2_SERVER_DEVENTRY,"EDServIP");
				//ret2 = aydevice2_update_domainip(AYDEVICE2_SERVER_PICYUN,"PicYunIP");
				//ret3 = aydevice2_update_domainip(AYDEVICE2_SERVER_UPYUN,"UPYunIP");
				if(ret1 < 0 || ret2 < 0 || ret3 < 0)
					domainip_interval_secs *= 2;
				if(domainip_interval_secs >= 300)
					domainip_interval_secs = 300;
			}
			t1 = t2;
		}
		ulumon_msg_check(2);
    }
    return 0;
}

void* log_process_thread(void *args)
{
    struct timespec t1,t2;
    char log[4096]={0};
    int line_size = 0,log_size = 0;
    char *plineend = NULL;
    int uplog_interval_secs = 30;// initial value

    set_thread_name("log_thread");
    clock_gettime(CLOCK_MONOTONIC,&t1);
    while(!sdk_get_handle(0)->exit_flag)
    {
	aydebug_vindicate_logfile();
	if(sdk_get_handle(0)->stream.status!=AYE_VAL_STREAM_STATUS_SENDDAT)
	{
	    clock_gettime(CLOCK_MONOTONIC,&t2);
	    if(t2.tv_sec - t1.tv_sec >= uplog_interval_secs)
	    {
			t1 = t2;
			log_size += aydebug_get_log(log+log_size,sizeof(log)-log_size);
			plineend = strrchr(log,'\n');
			if(plineend!=NULL)
			{
				*plineend = '\0';
				line_size = strlen(log);

				if(aydevice2_upload_log((const char *)log)>=0)
				{
					uplog_interval_secs = uplog_interval_secs+30 < 300?uplog_interval_secs+30:300;
				}
				log_size -= (line_size+1);
				memmove(log,plineend+1,log_size);
				line_size = 0;
			}
	    }
	}
	sleep(5);
    }
    return NULL;
}

