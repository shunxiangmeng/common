#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "aystream.h"
#include "ay_sdk.h"
#include "protocol_device.h"
#include "monitor.h"
#include "cJSON.h"
#include "version.h"

struct net_statistic_info net_stat_info;

struct device_sdk_running_info
{
    Dev_info_struct devinfo;
    Chnl_ctrl_table_struct chninfo[MAX_CHANNEL_NUM];
    Connect_Info    strconn[MAX_STREAM_THREAD_NUM];
    ULK_Alarm_Struct	alarminfo;
    struct net_statistic_info netinfo;
};

int trace_init_net_stat_info(void)
{
    int i = 0;
    memset(&net_stat_info,0,sizeof(net_stat_info));
    for(i=0;i<5;i++)
    {
	clock_gettime(CLOCK_MONOTONIC,&net_stat_info.last_ts[i]);
    }
    return 0;
}
int trace_get_net_stat_info(struct net_statistic_info *netinfo)
{
    if(netinfo!=NULL)
    {
	memcpy(netinfo,&net_stat_info,sizeof(net_stat_info));
	return 0;
    }
    return -1;
}
int trace_update_net_stat_info(uint32 tosndbytes,uint32 sndokbytes, uint32 rcvbytes)
{
    struct timespec now_ts;
    uint32 tgap = 0;

    clock_gettime(CLOCK_MONOTONIC,&now_ts);
    tgap = now_ts.tv_sec - net_stat_info.last_ts[0].tv_sec;

    int i = 0;
    for(i=0;i<5;i++)
    {
	net_stat_info.bytes_tosend[i] += tosndbytes;
	net_stat_info.bytes_sendok[i] += sndokbytes;
	net_stat_info.bytes_recvok[i] += rcvbytes;
    }

    net_stat_info.want_send_speed[0] = net_stat_info.bytes_tosend[0]*1.0/(1024*tgap);
    net_stat_info.real_send_speed[0] = net_stat_info.bytes_sendok[0]*1.0/(1024*tgap);
    if((tgap=now_ts.tv_sec - net_stat_info.last_ts[1].tv_sec) >= 10)
    {
	net_stat_info.last_ts[1] = now_ts;
	net_stat_info.want_send_speed[1] = net_stat_info.bytes_tosend[1]*1.0/(1024*tgap);
	net_stat_info.real_send_speed[1] = net_stat_info.bytes_sendok[1]*1.0/(1024*tgap);
	net_stat_info.bytes_tosend[1] = 0;
	net_stat_info.bytes_sendok[1] = 0;
	net_stat_info.bytes_recvok[1] = 0;
    }
    if((tgap=now_ts.tv_sec - net_stat_info.last_ts[2].tv_sec) >= 5*60)
    {
	net_stat_info.last_ts[2] = now_ts;
	net_stat_info.want_send_speed[2] = net_stat_info.bytes_tosend[2]*1.0/(1024*tgap);
	net_stat_info.real_send_speed[2] = net_stat_info.bytes_sendok[2]*1.0/(1024*tgap);
	net_stat_info.bytes_tosend[2] = 0;
	net_stat_info.bytes_sendok[2] = 0;
	net_stat_info.bytes_recvok[2] = 0;
    }
    if((tgap=now_ts.tv_sec - net_stat_info.last_ts[3].tv_sec) >= 10*60)
    {
	net_stat_info.last_ts[3] = now_ts;
	net_stat_info.want_send_speed[3] = net_stat_info.bytes_tosend[3]*1.0/(1024*tgap);
	net_stat_info.real_send_speed[3] = net_stat_info.bytes_sendok[3]*1.0/(1024*tgap);
	net_stat_info.bytes_tosend[3] = 0;
	net_stat_info.bytes_sendok[3] = 0;
	net_stat_info.bytes_recvok[3] = 0;
    }
    if((tgap=now_ts.tv_sec - net_stat_info.last_ts[4].tv_sec) >= 30*60)
    {
	net_stat_info.last_ts[4] = now_ts;
	net_stat_info.want_send_speed[4] = net_stat_info.bytes_tosend[4]*1.0/(1024*tgap);
	net_stat_info.real_send_speed[4] = net_stat_info.bytes_sendok[4]*1.0/(1024*tgap);
	net_stat_info.bytes_tosend[4] = 0;
	net_stat_info.bytes_sendok[4] = 0;
	net_stat_info.bytes_recvok[4] = 0;
    }
    return 0;
}

int Ulu_SDK_Query_Run_Info(struct device_sdk_running_info *info)
{
    if(info == NULL)
	return -1;

    memset(info,0,sizeof(*info));
    memcpy(&info->devinfo,&sdk_get_handle(0)->devinfo,sizeof(info->devinfo));
    memcpy(&info->chninfo[0],&sdk_get_handle(0)->devobj[0].streamFilter[0],sizeof(info->chninfo));
    memcpy(&info->strconn[0],&sdk_get_handle(0)->stream.conns[0],sizeof(info->strconn));
    memcpy(&info->alarminfo,&sdk_get_handle(0)->alarm_ctrl,sizeof(info->alarminfo));
    memcpy(&info->netinfo,&net_stat_info,sizeof(info->netinfo));

    return 0;
}

ayJSON *create_devinfo_json(Dev_info_struct *info)
{
    ayJSON *mjson = NULL, *item = NULL;
    mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	char strdesc[128] = {0};
	struct in_addr ip;
	snprintf(strdesc,sizeof(strdesc),"%#x",info->dev_status_mask);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"DevStatusMask",item);
	ip.s_addr = info->local_net_ip;
	snprintf(strdesc,sizeof(strdesc),"%s",inet_ntoa(ip));
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"local_net_ip",item);
	item = ayJSON_CreateNumber((double)info->entry_timeout);
	ayJSON_AddItemToObject(mjson,"entry_timeout",item);
	item = ayJSON_CreateNumber((double)info->max_rate);
	ayJSON_AddItemToObject(mjson,"MaxRate",item);
	item = ayJSON_CreateNumber((double)info->min_rate);
	ayJSON_AddItemToObject(mjson,"MinRate",item);

	item = ayJSON_CreateNumber((double)info->video_width);
	ayJSON_AddItemToObject(mjson,"video_width",item);
	item = ayJSON_CreateNumber((double)info->video_height);
	ayJSON_AddItemToObject(mjson,"video_height",item);
	item = ayJSON_CreateNumber((double)info->audio_type);
	ayJSON_AddItemToObject(mjson,"audio_type",item);
	item = ayJSON_CreateNumber((double)info->audio_chnl);
	ayJSON_AddItemToObject(mjson,"audio_chnl",item);
	item = ayJSON_CreateNumber((double)info->audio_smaple_rt);
	ayJSON_AddItemToObject(mjson,"audio_smaple_rt",item);
	item = ayJSON_CreateNumber((double)info->audio_bit_width);
	ayJSON_AddItemToObject(mjson,"audio_bit_width",item);
	item = ayJSON_CreateNumber((double)info->channelnum);
	ayJSON_AddItemToObject(mjson,"channelnum",item);
	item = ayJSON_CreateNumber((double)info->dev_type);
	ayJSON_AddItemToObject(mjson,"dev_type",item);
	item = ayJSON_CreateNumber((double)info->ptz_flag);
	ayJSON_AddItemToObject(mjson,"ptz_flag",item);
	item = ayJSON_CreateNumber((double)info->mic_flag);
	ayJSON_AddItemToObject(mjson,"mic_flag",item);
	item = ayJSON_CreateNumber((double)info->can_rec_voice);
	ayJSON_AddItemToObject(mjson,"can_rec_voice",item);
	item = ayJSON_CreateNumber((double)info->upload_ctrl);
	ayJSON_AddItemToObject(mjson,"upload_ctrl",item);
	item = ayJSON_CreateNumber((double)info->hard_disk);
	ayJSON_AddItemToObject(mjson,"hard_disk",item);
	item = ayJSON_CreateNumber((double)info->has_tfcard);
	ayJSON_AddItemToObject(mjson,"has_tfcard",item);
	item = ayJSON_CreateNumber((double)info->wifi_online_broad);
	ayJSON_AddItemToObject(mjson,"wifi_online_broad",item);

	item = ayJSON_CreateString(info->rw_path);
	ayJSON_AddItemToObject(mjson,"rw_path",item);
	item = ayJSON_CreateString(info->device_id_str);
	ayJSON_AddItemToObject(mjson,"device_id_str",item);
	snprintf(strdesc,sizeof(strdesc),"id=>%d,sn=>%s,name=>%s",info->Sn_info.OEMID,info->Sn_info.SN,info->Sn_info.OEM_name);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"OEM Info",item);
	item = ayJSON_CreateNumber((double)info->use_type);
	ayJSON_AddItemToObject(mjson,"use_type",item);

	item = ayJSON_CreateNumber((double)info->resp_json_code);
	ayJSON_AddItemToObject(mjson,"resp_json_code",item);
	snprintf(strdesc,sizeof(strdesc),"%#x",info->vip_flag);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"vip_flag",item);
	item = ayJSON_CreateNumber((double)info->strm_conn_stat);
	ayJSON_AddItemToObject(mjson,"StrmConnStat",item);
	ip.s_addr = info->stream_serv_ip;
	snprintf(strdesc,sizeof(strdesc),"%s:%hu",inet_ntoa(ip),info->stream_serv_port);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"StreamServer",item);

	//snprintf(strdesc,sizeof(strdesc),"%s(%d)",info->token.token_bin,info->token.token_bin_length);
	//item = ayJSON_CreateString(strdesc);
	item = ayJSON_CreateNumber((double)info->token.token_bin_length);
	ayJSON_AddItemToObject(mjson,"stream token_bin_length",item);
	item = ayJSON_CreateNumber((double)info->token_update_flag);
	ayJSON_AddItemToObject(mjson,"token_update_flag",item);

	ip.s_addr = info->public_net_ip;
	snprintf(strdesc,sizeof(strdesc),"%s:%hu",inet_ntoa(ip),info->public_net_port);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"PublicNetAddr",item);
	item = ayJSON_CreateNumber((double)info->expected_cycle);
	ayJSON_AddItemToObject(mjson,"ExpectedCycle",item);
	item = ayJSON_CreateNumber((double)info->expt_cycle_strm);
	ayJSON_AddItemToObject(mjson,"ExptCycleStrm",item);

	//snprintf(strdesc,sizeof(strdesc),"%s(%d)",info->entry_token_b64.token_bin,info->entry_token_b64.token_bin_length);
	item = ayJSON_CreateString(info->entry_token_base64);
	ayJSON_AddItemToObject(mjson,"entry token",item);

	item = ayJSON_CreateNumber((double)info->sync_time_flag);
	ayJSON_AddItemToObject(mjson,"sync_time_flag",item);
	item = ayJSON_CreateString(info->rtsp_url);
	ayJSON_AddItemToObject(mjson,"rtsp_url",item);
	item = ayJSON_CreateNumber((double)info->sdcard_status);
	ayJSON_AddItemToObject(mjson,"sdcard_status",item);
	item = ayJSON_CreateNumber((double)info->record_mode);
	ayJSON_AddItemToObject(mjson,"record_mode",item);
    }
    return mjson;
}
ayJSON *create_chninfo_json(Chnl_ctrl_table_struct *info)
{
    ayJSON *mjson = NULL, *item = NULL;

    mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	int bit_r[4]={0};
	int bit_his_r = info->bit_his_r;
	int i = 0;
	for(i = 0;i<4;i++)
	    bit_r[i] = info->bit_r[i];
	item = ayJSON_CreateIntArray(bit_r,4);
	ayJSON_AddItemToObject(mjson,"bitrateCtrl",item);
	item = ayJSON_CreateNumber(bit_his_r);
	ayJSON_AddItemToObject(mjson,"histCtrl",item);
    }
    return mjson;
}
ayJSON *create_streamconn_json(Connect_Info info[],int num)
{
    ayJSON *mjson = NULL, *item = NULL;
    mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	int fd[MAX_STREAM_THREAD_NUM]={0};
	int i = 0;
	for(i = 0;i<num;i++)
	    fd[i] = info[i].socket_fd;
	item = ayJSON_CreateIntArray(fd,MAX_STREAM_THREAD_NUM);
	ayJSON_AddItemToObject(mjson,"sockets",item);
    }
    return mjson;
}
ayJSON *create_ipconf_json(st_ay_entry_ctrl *info)
{
    ayJSON *mjson = NULL, *item = NULL;

    mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	item = ayJSON_CreateNumber((double)info->num);
	ayJSON_AddItemToObject(mjson,"IPNum",item);

	int i = 0;
	char strdesc[5][32];
	struct in_addr ip;
	for(i = 0;i<info->num;i++)
	{
	    ip.s_addr = info->addr[i].ip;
	    snprintf(strdesc[i],sizeof(strdesc[i]),"%s:%hu",inet_ntoa(ip),info->addr[i].port);
	}
	item = ayJSON_CreateStringArray((const char **)strdesc,info->num);
	ayJSON_AddItemToObject(mjson,"IPAddrs",item);
    }
    return mjson;
}

char *Ulu_SDK_Get_JsonInfo(int format)
{
    struct device_sdk_running_info *info;
    char *jsontext = NULL;

    struct device_sdk_running_info sdkinfo;
    Ulu_SDK_Query_Run_Info(&sdkinfo);

    info = &sdkinfo;
    ayJSON *mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	ayJSON *item = NULL;
	item = create_devinfo_json(&info->devinfo);
	ayJSON_AddItemToObject(mjson,"DevInfo",item);

	item = create_chninfo_json(&info->chninfo[0]);
	ayJSON_AddItemToObject(mjson,"ChnInfo",item);

	item = create_streamconn_json(info->strconn,MAX_STREAM_THREAD_NUM);
	ayJSON_AddItemToObject(mjson,"Streamconn",item);

	//todo...
	//ayJSON_AddItemToObject(mjson,"Alarminfo",item);

	//todo...
	//ayJSON_AddItemToObject(mjson,"NetInfo",item);

	if(format)
	    jsontext = ayJSON_Print(mjson);
	else
	    jsontext = ayJSON_PrintUnformatted(mjson);
	ayJSON_Delete(mjson);
    }
    return jsontext;
}

int Ulu_SDK_Refresh_Device_Info(const char *filename)
{
    if(filename==NULL)
	return -1;
    FILE *fp = fopen(filename,"w");
    if(fp == NULL)
	return -1;

    char *jsontext = Ulu_SDK_Get_JsonInfo(1);
    if(jsontext!=NULL)
    {
	fwrite(jsontext,strlen(jsontext),1,fp);
	free(jsontext);
    }
    fflush(fp);
    fclose(fp);
    return 0;
}

char *aymonitor_get_json_log(int format,const char *event,const char *msg)
{
    struct device_sdk_running_info *info;
    char *jsontext = NULL;

    struct device_sdk_running_info sdkinfo;
    Ulu_SDK_Query_Run_Info(&sdkinfo);

    info = &sdkinfo;
    ayJSON *mjson = ayJSON_CreateObject();
    if(mjson != NULL)
    {
	struct in_addr ip;
	char strdesc[512];
	ayJSON *item = NULL;

	/****** Must have **********/
	snprintf(strdesc,sizeof(strdesc),"%ld",time(NULL));
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"time_utc",item);

	item = ayJSON_CreateString(event);
	ayJSON_AddItemToObject(mjson,"event",item);

	item = ayJSON_CreateString(info->devinfo.device_id_str);
	ayJSON_AddItemToObject(mjson,"did",item);

	snprintf(strdesc,sizeof(strdesc),"%d",info->devinfo.channelnum);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"channel",item);

	item = ayJSON_CreateString(RVERSION);
	ayJSON_AddItemToObject(mjson,"ver",item);

	/******* Optional Data *******/
	switch(info->devinfo.dev_type)
	{
	    case 1:snprintf(strdesc,sizeof(strdesc),"dvr");break;
	    case 2:snprintf(strdesc,sizeof(strdesc),"nvr");break;
	    case 3:snprintf(strdesc,sizeof(strdesc),"ipc");break;
	    case 4:snprintf(strdesc,sizeof(strdesc),"box");break;
	    default:snprintf(strdesc,sizeof(strdesc),"unknown");break;
	}
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"dev_type",item);

	item = ayJSON_CreateString(info->devinfo.entry_token_base64);
	ayJSON_AddItemToObject(mjson,"dev_token",item);

	ip.s_addr = info->devinfo.stream_serv_ip;
	item = ayJSON_CreateString(inet_ntoa(ip));
	ayJSON_AddItemToObject(mjson,"stream_ip",item);

	if(msg!=NULL)
	{
	    item = ayJSON_CreateString(msg);
	    ayJSON_AddItemToObject(mjson,"adder",item);
	}

	item = ayJSON_CreateString(info->devinfo.Sn_info.SN);
	ayJSON_AddItemToObject(mjson,"sn",item);

	snprintf(strdesc,sizeof(strdesc),"%d,%d",info->devinfo.min_rate,info->devinfo.max_rate);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"rate",item);

	snprintf(strdesc,sizeof(strdesc),"%d",info->devinfo.audio_type);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"audio",item);

	snprintf(strdesc,sizeof(strdesc),"%d",info->devinfo.ptz_flag);
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"zoom",item);

	item = ayJSON_CreateString("0");
	ayJSON_AddItemToObject(mjson,"webcont",item);

	snprintf(strdesc,sizeof(strdesc),"%s",info->devinfo.hard_disk||info->devinfo.has_tfcard?"1":"0");
	item = ayJSON_CreateString(strdesc);
	ayJSON_AddItemToObject(mjson,"disk",item);

	// local ip
	ip.s_addr = info->devinfo.local_net_ip;
	item = ayJSON_CreateString(inet_ntoa(ip));
	ayJSON_AddItemToObject(mjson,"intarip",item);

	//item = ayJSON_CreateString(strdesc);
	//ayJSON_AddItemToObject(mjson,"cpu",item);

	//item = ayJSON_CreateString(strdesc);
	//ayJSON_AddItemToObject(mjson,"payload",item);

	item = ayJSON_CreateString(info->devinfo.entry_ver);
	ayJSON_AddItemToObject(mjson,"entry_ver",item);

	if(format)
	{
	    jsontext = ayJSON_Print(mjson);
	}
	else
	{
	    jsontext = ayJSON_PrintUnformatted(mjson);
	}
	ayJSON_Delete(mjson);
    }
    return jsontext;
}

int aymonitor_get_uplog(time_t htime,const char *event,const char *msg, char *ptext,int size)
{
    struct device_sdk_running_info *info;
    struct  in_addr saddr;
    int ret = 0,length = 0;
    struct device_sdk_running_info sdkinfo;
    Ulu_SDK_Query_Run_Info(&sdkinfo);

    info = &sdkinfo;
    if(ptext != NULL)
    {
	/****** Must have **********/
	ret = snprintf(ptext,size,"%s|%ld|%s|",event,htime,info->devinfo.device_id_str);

	if(strcmp(event,"DeviceCommEvent")==0)
	{// oemname|oemsn|status|comment|
	    ret += snprintf(ptext+ret,size-ret,"%s|%s|%s|",info->devinfo.Sn_info.OEM_name,info->devinfo.Sn_info.SN,msg);
	}
	else if(strcmp(event,"TrackerConnEvent")==0)
	{// TrackerAddrs|status|comment|
	    ret += snprintf(ptext+ret,size-ret,"%s|",msg);
	}
	else if(strcmp(event,"StreamConnEvent")==0)
	{// StreamAddrs|status|comment|
	    saddr.s_addr = info->devinfo.stream_serv_ip;
	    ret += snprintf(ptext+ret,size-ret,"%s|%s|",inet_ntoa(saddr),msg);
	}
	else if(strcmp(event,"MediaEvent")==0)
	{// ChannelID|UploadRate|StreamAddrs|SessionType|MediaType|status|comment|
	    ret += snprintf(ptext+ret,size-ret,"%s|",msg);
	}
	else if(strcmp(event,"CmdEvent")==0)
	{// sender|Command|comment|
	    ret += snprintf(ptext+ret,size-ret,"%s|",msg);
	}

	ret += snprintf(ptext+ret,size-ret,"\n");
	length = strlen(ptext);
    }
    return length;
}
