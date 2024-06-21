#include <stdio.h>
#include <stdlib.h>

#include "ghttp.h"
#include "define.h"
#include "ayutil.h"
#include "ay_sdk.h"
#include "hd_adapter.h"
#include "protocol_device.h"
#include "AnyanDeviceMng.h"
#include "Anyan_Device_SDK.h"

static struct dm_cmd_callback dm_cbfuncs;

void ADM_Set_CmdCallback(struct dm_cmd_callback cbfuncs)
{
    dm_cbfuncs = cbfuncs;
}

/*
 * return webcontent use malloc memory,so the caller must free the RESULT if it's not NULL!!
 */
char *ADM_Get_Webcontent_Helper(const char *purl, int *pout_len)
{
    int bytes = 0;
    ghttp_status req_status;
    ghttp_request *pReq = NULL;
    char *pWebContent = NULL;
    char *pcontent = NULL;

    if (purl == NULL || NULL==(pReq=ghttp_request_new()))
    {
	DEBUGF("ghttp_request_new fail!\n");
	return NULL;
    }

    if (ghttp_set_uri(pReq,(char*)purl) < 0)
    {
	DEBUGF("ghttp_set_uri:%s fail!\n",purl);
	ghttp_request_destroy(pReq);
	return NULL;
    }

    ghttp_prepare(pReq);
    ghttp_set_sync(pReq, ghttp_async);
    do {
	req_status = ghttp_process(pReq);
	if (req_status == ghttp_error)
	{
	    DEBUGF("ghttp err: %s\n", ghttp_get_error(pReq));
	    ghttp_request_destroy(pReq);
	    return NULL;
	}

	if (req_status != ghttp_error && ghttp_get_body_len(pReq) > 0) 
	{
	    bytes += ghttp_get_body_len(pReq);
	}
    } while (req_status == ghttp_not_done);

    bytes = ghttp_get_body_len(pReq);
    if((pWebContent = ghttp_get_body(pReq)))
    {
	pcontent = (char *)malloc(bytes+1);
	if(NULL != pcontent)
	{
	    memset(pcontent, 0, bytes + 1);
	    memcpy(pcontent,pWebContent,bytes);
	    if(pout_len) *pout_len = bytes;
	}
    }

    ghttp_request_destroy(pReq);
    return pcontent;
}

int hd_res_operation_unsupport(HD_DEV_HANDLE hdh, const char *ack_cmd)
{
    char *preq = NULL;
    int ret = 0;

    preq = hd_build_ack(ack_cmd,hdh->last_serial,E_DEV_ERROR_OPERATION_NOT_SUPPORT,"operation unsupported",NULL,0);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	free(preq);
	return ret;
    }
    return -1;
}

int hd_req_get_device_version(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    struct hd_cmd_dict data[3];
    char *preq = NULL;
    int ret = 0;
    
    if(dm_cbfuncs.cmd_req_get_device_version == NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"get_device_version_ack");
	return ret;
    }

    struct dm_get_device_version_req dmReq;
    struct dm_get_device_version_ack dmRes;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    dm_cbfuncs.cmd_req_get_device_version(&dmReq,&dmRes);

    strcpy(data[0].name , "version");
    data[0].type = 1;
    strncpy(data[0].string,dmRes.version,63);
    strcpy(data[1].name , "devsn");
    data[1].type = 1;
    memset(data[1].string,0,sizeof(data[1].string));
    strncpy(data[1].string,hdh->devid+2,16);
    strcpy(data[2].name , "ulusn");
    data[2].type = 1;
    strcpy(data[2].string,hdh->devid);

    preq = hd_build_ack("get_device_version_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,data,3);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	free(preq);
	return ret;
    }
    return -1;
}

int hd_req_start_update(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(dm_cbfuncs.cmd_req_start_update == NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"start_update_ack");
	return ret;
    }

    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"url");
    if(pitem!=NULL)
    {
	struct dm_start_update_req dmReq;
	struct dm_start_update_ack dmRes;
    
	memset((void*)&dmReq,0,sizeof(dmReq));
	memset((void*)&dmRes,0,sizeof(dmRes));
	strncpy(dmReq.url,pitem->valuestring,127);
	pitem = ayJSON_GetObjectItem(pdata,"check");
	if(pitem)
	{
	    ayJSON *ptemp = ayJSON_GetObjectItem(pitem,"type");
	    strncpy(dmReq.check.type,ptemp->valuestring,63);
	    ptemp = ayJSON_GetObjectItem(pitem,"value");
	    strncpy(dmReq.check.value,ptemp->valuestring,63);
	    ptemp = ayJSON_GetObjectItem(pitem,"size");
	    dmReq.check.size = ptemp->valueint;
	}
	DEBUGF("update url:%s\n",dmReq.url);

	dm_cbfuncs.cmd_req_start_update(&dmReq,&dmRes);
	preq = hd_build_ack_just_ok("start_update_ack",hdh->last_serial);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }

    return ret;
}


int hd_push_device_log(int sock, const char *token,const char *log)
{
    struct hd_cmd_dict data[2];
    char *preq = NULL;
    int ret = 0;

    strcpy(data[0].name , "token");
    data[0].type = 1;
    strcpy(data[0].string,token);
    strcpy(data[1].name , "log");
    data[1].type = 1;
    strcpy(data[1].string,log);
    preq = hd_build_req("device_log_push","ulucuanyan",data,1);
    if(preq!=NULL)
    {
	ret = send(sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) ret = -1;
	free(preq);
    }
    return ret;
}

static int g_hd_debug_flag = 0;
static char g_hd_debug_token[128];
void *hd_process_debug(void *arg)
{
    int sock = (int)arg;
    int len = 0,ret = 0;
    char log[1024];

    while(g_hd_debug_flag)
    {
	memset(log,0,sizeof(log));
	//len = aydebug_get_log(log,sizeof(log)); // FIXME
	if(len > 0)
	{
	    ret = hd_push_device_log(sock,g_hd_debug_token,log);
	    if(ret < 0)
	    {
		DEBUGF("hd_push_device_log fail!\n");
		break;
	    }
	}
	usleep(500000);
    }
    close(sock);
    g_hd_debug_flag = 0;
    return NULL;
}

int hd_process_open_debug(HD_DEV_HANDLE hdh,int sock,const char *token)
{
    if(g_hd_debug_flag)
    {
	DEBUGF("has open debug...\n");
	close(sock);
	return 0;
    }
    pthread_t debug_tid;
    g_hd_debug_flag = 1;
    strcpy(g_hd_debug_token,token);
    pthread_create(&debug_tid,NULL,hd_process_debug,(void*)sock);
    pthread_detach(debug_tid);
    
    return 0;
}

int hd_process_close_debug(HD_DEV_HANDLE hdh)
{
    g_hd_debug_flag = 0;
    return 0;
}

int hd_req_open_debug(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    char server[32];
    char token[128];
    unsigned short port = 0;
    int ret = 0;

    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"server");
    if(pitem!=NULL)
    {
	sscanf(pitem->valuestring,"%[0-9.]:%hu",server,&port);

	pitem = ayJSON_GetObjectItem(pdata,"token");
	strcpy(token,pitem->valuestring);

	int sock = -1;
	sock = ayutil_tcp_client(server,port,5);
	if(sock < 0)
	{
	    DEBUGF("connect [%s:%d] fail!\n",server,port);
	    preq = hd_build_ack("open_debug_ack",hdh->last_serial,E_DEV_ERROR_PARA_INVALID,"connect fail",0,0);
	}
	else
	{
	    hd_process_open_debug(hdh,sock,token);
	    preq = hd_build_ack_just_ok("open_debug_ack",hdh->last_serial);
	}
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}
int hd_req_close_debug(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    hd_process_close_debug(hdh);
    preq = hd_build_ack_just_ok("close_debug_ack",hdh->last_serial);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	free(preq);
	return ret;
    }
    return -1;
}


int hd_req_start_screenshot(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    struct hd_cmd_dict data[8];
    char *preq = NULL;
    int ret = 0;
    char devid[128];
    int channel = 0;

    if(dm_cbfuncs.cmd_req_start_screenshot == NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"start_screenshot_ack");
	return ret;
    }

    struct dm_start_screenshot_req dmReq;
    struct dm_start_screenshot_ack dmRes;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));

    // "data":{"channel":0,"device_id":10494}
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"device_id");
    if(pitem!=NULL)
    {
	snprintf(devid,sizeof(devid),"%d",pitem->valueint);
    }

    pitem = ayJSON_GetObjectItem(pdata,"channel");
    channel = pitem->valueint;
    
    dmReq.channel = channel;
    dm_cbfuncs.cmd_req_start_screenshot(&dmReq,&dmRes);

    strcpy(data[0].name , "image_name");
    data[0].type = 1;
    strcpy(data[0].string,dmRes.image_name);
    strcpy(data[1].name , "image_size");
    data[1].type = 0;
    data[1].value = dmRes.image_size;
    strcpy(data[2].name , "cloud_storage_path");
    data[2].type = 1;
    strcpy(data[2].string,dmRes.cloud_storage_path);
    strcpy(data[3].name , "cloud_storage_type");
    data[3].type = 0;
    data[3].value = dmRes.cloud_storage_type;
    strcpy(data[4].name , "excute_time");
    data[4].type = 0;
    data[4].value = dmRes.excute_time;
    strcpy(data[5].name , "upload_start_time");
    data[5].type = 0;
    data[5].value = dmRes.upload_start_time;
    strcpy(data[6].name , "upload_end_time");
    data[6].type = 0;
    data[6].value = dmRes.upload_end_time;
    strcpy(data[7].name , "channel");
    data[7].type = 0;
    data[7].value = channel;

    preq = hd_build_ack("start_screenshot_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,data,8);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	free(preq);
	return ret;
    }
    return -1;
}

int hd_req_query_record(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;
    int channel = 0;

    if(dm_cbfuncs.cmd_req_query_record==NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"query_record_ack");
	return ret;
    }

    struct dm_query_record_req dmReq;
    struct dm_query_record_ack dmRes;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	time_t s_tm = 0,e_tm = 0;
	channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"s_tm");
	if(pitem!=NULL)
	{
	    s_tm = pitem->valueint;
	}
	pitem = ayJSON_GetObjectItem(pdata,"e_tm");
	if(pitem!=NULL)
	{
	    e_tm = pitem->valueint;
	}

	dmReq.channel = channel;
	dmReq.s_tm = s_tm;
	dmReq.e_tm = e_tm;

	struct hd_cmd_dict data[1];
	int list_num = 0;
	struct dm_record_list *res_list = dmRes.record_list;

	dm_cbfuncs.cmd_req_query_record(&dmReq,&dmRes);

	list_num = dmRes.list_num;
	strcpy(data[0].name , "record_list");
	data[0].type = 3;
	data[0].value = list_num; 
	data[0].array_len = 4; //list member number

	int i = 0,j = 4;
	struct hd_cmd_dict *plist = calloc(data[0].array_len*data[0].value,sizeof(struct hd_cmd_dict));	
	if(plist==NULL)
	{
	    preq = hd_build_ack("query_record_ack",hdh->last_serial,E_DEV_ERROR_INTERNAL,"malloc fail",0,0);
	}
	else
	{
	    for(i=0;i<data[0].value;i++)
	    {
		strcpy(plist[i*j+0].name , "record_name");
		plist[i*j+0].type = 1;
		strcpy(plist[i*j+0].string,res_list[i].record_name);
		strcpy(plist[i*j+1].name , "size");
		plist[i*j+1].type = 0;
		plist[i*j+1].value = res_list[i].size;
		strcpy(plist[i*j+2].name , "s_tm");
		plist[i*j+2].type = 0;
		plist[i*j+2].value = res_list[i].s_tm;
		strcpy(plist[i*j+3].name , "e_tm");
		plist[i*j+3].type = 0;
		plist[i*j+3].value = res_list[i].e_tm;
	    }
	    data[0].parray = plist;

	    preq = hd_build_ack("query_record_ack",hdh->last_serial,0,"ok",data,sizeof(data)/sizeof(struct hd_cmd_dict));
	    free(plist);
	    plist = NULL;
	}

	if(dmRes.free_need)
	{
	    free(dmRes.record_list);
	}
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	}
	return ret;
    }

    return ret;
}

int hd_req_start_download(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(dm_cbfuncs.cmd_req_start_download == NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"start_download_ack");
	return ret;
    }

    struct dm_start_download_req dmReq ;
    struct dm_start_download_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"token");
    if(pitem!=NULL)
    {
	strncpy(dmReq.token,pitem->valuestring,127);

	pitem = ayJSON_GetObjectItem(pdata,"channel");
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"record_name");
	strcpy(dmReq.record_name,pitem->valuestring);
	pitem = ayJSON_GetObjectItem(pdata,"s_tm");
	dmReq.s_tm = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"e_tm");
	dmReq.e_tm = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"size");
	dmReq.size = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"audio_frame_index");
	dmReq.audio_frame_index = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"video_frame_index");
	dmReq.video_frame_index = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"server");
	sscanf(pitem->valuestring,"%[0-9.]:%hd",dmReq.server_ip,&dmReq.server_port);
	pitem = ayJSON_GetObjectItem(pdata,"transport_protocol");
	strcpy(dmReq.transport_protocol,pitem->valuestring);

	dm_cbfuncs.cmd_req_start_download(&dmReq,&dmRes);

	preq = hd_build_ack_just_ok("start_download_ack",hdh->last_serial);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_stop_download(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_stop_download)
    {
	ret = hd_res_operation_unsupport(hdh,"stop_download_ack");
	return ret;
    }
    struct dm_stop_download_req dmReq ;
    struct dm_stop_download_ack dmRes ;
    
    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"record_name");
	strcpy(dmReq.record_name,pitem->valuestring);
	pitem = ayJSON_GetObjectItem(pdata,"s_tm");
	dmReq.s_tm = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"e_tm");
	dmReq.e_tm = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"size");
	dmReq.size = pitem->valueint;

	dm_cbfuncs.cmd_req_stop_download(&dmReq,&dmRes);

	preq = hd_build_ack_just_ok("stop_download_ack",hdh->last_serial);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_direction_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_direction_ctrl)
    {
	ret = hd_res_operation_unsupport(hdh,"direction_ctrl_ack");
	return ret;
    }
    struct dm_direction_ctrl_req dmReq ;
    struct dm_direction_ctrl_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"direction");
	strcpy(dmReq.direction,pitem->valuestring);
	pitem = ayJSON_GetObjectItem(pdata,"speed");
	dmReq.speed = pitem->valueint;

	dm_cbfuncs.cmd_req_direction_ctrl(&dmReq,&dmRes);

	preq = hd_build_ack("direction_ctrl_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_stop_ptzctrl(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_stop_ctrl)
    {
	ret = hd_res_operation_unsupport(hdh,"stop_ctrl_ack");
	return ret;
    }
    struct dm_stop_ctrl_req dmReq ;
    struct dm_stop_ctrl_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;

	dm_cbfuncs.cmd_req_stop_ctrl(&dmReq,&dmRes);

	preq = hd_build_ack("stop_ctrl_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_set_motion_detect(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(dm_cbfuncs.cmd_req_set_motion_detect==NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"set_motion_detect_ack");
	return ret;
    }
    int num = 0,i = 0;
    struct dm_set_motion_detect_req dmReq;
    struct dm_set_motion_detect_ack dmRes;

    memset(&dmReq,0,sizeof(dmReq));
    memset(&dmRes,0,sizeof(dmRes));

    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"sub_channel");
	if(pitem) dmReq.sub_channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"sensitivity");
	if(pitem!=NULL) dmReq.sensitivity = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"switch");
	if(pitem!=NULL) dmReq.enable = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"snapshot_switch");
	if(pitem!=NULL) dmReq.snapshot_enable = pitem->valueint;
	
	pitem = ayJSON_GetObjectItem(pdata,"period_list");
	if(pitem!=NULL && (num=ayJSON_GetArraySize(pitem))>0)
	{
	    int w = 0;
	    dmReq.period_num = (num>7)?7:num;
	    for(i=0;i<num;i++)
	    {
		ayJSON *punit = ayJSON_GetArrayItem(pitem,i);
		ayJSON *pinfo = ayJSON_GetObjectItem(punit,"week");
		if(pinfo!=NULL)
		{
		    w = pinfo->valueint;
		    if(w> 0 && w<8)
		    {// ensure week in 1~7
			dmReq.period_list[w-1].week = w;
			pinfo = ayJSON_GetObjectItem(punit,"start");
			strcpy(dmReq.period_list[w-1].start,pinfo->valuestring);
			pinfo = ayJSON_GetObjectItem(punit,"end");
			strcpy(dmReq.period_list[w-1].end,pinfo->valuestring);
		    }
		}
	    }
	    pitem = ayJSON_GetObjectItem(pdata,"region_list");
	    if(pitem!=NULL && (num=ayJSON_GetArraySize(pitem))>0)
	    {
		dmReq.region_num = (num>16)?16:num;
		for(i=0;i<num;i++)
		{
		    ayJSON *punit = ayJSON_GetArrayItem(pitem,i);
		    ayJSON *pinfo = ayJSON_GetObjectItem(punit,"left");
		    if(pinfo!=NULL) dmReq.region_list[i].left = pinfo->valueint;
		    pinfo = ayJSON_GetObjectItem(punit,"top");
		    if(pinfo!=NULL) dmReq.region_list[i].top = pinfo->valueint;
		    pinfo = ayJSON_GetObjectItem(punit,"right");
		    if(pinfo!=NULL) dmReq.region_list[i].right = pinfo->valueint;
		    pinfo = ayJSON_GetObjectItem(punit,"bottom");
		    if(pinfo!=NULL) dmReq.region_list[i].bottom= pinfo->valueint;
		}
	    }
	}
	dm_cbfuncs.cmd_req_set_motion_detect(&dmReq,&dmRes);
	preq = hd_build_ack("set_motion_detect_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_get_motion_detect(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(dm_cbfuncs.cmd_req_get_motion_detect==NULL)
    {
	ret = hd_res_operation_unsupport(hdh,"get_motion_detect_ack");
	return ret;
    }
    int i = 0;
    struct dm_get_motion_detect_req dmReq;
    struct dm_get_motion_detect_ack dmRes;

    memset(&dmReq,0,sizeof(dmReq));
    memset(&dmRes,0,sizeof(dmRes));

    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"sub_channel");
	if(pitem) dmReq.sub_channel = pitem->valueint;

	dm_cbfuncs.cmd_req_get_motion_detect(&dmReq,&dmRes);

	struct hd_cmd_dict data[5];
	strcpy(data[0].name , "sensitivity");
	data[0].type = 0;
	data[0].value = dmRes.sensitivity; 
	strcpy(data[1].name , "switch");
	data[1].type = 0;
	data[1].value = dmRes.enable; 
	strcpy(data[2].name , "snapshot_switch");
	data[2].type = 0;
	data[2].value = dmRes.snapshot_enable; 
	strcpy(data[3].name , "period_list");
	data[3].type = 3;
	data[3].array_len = 3; 
	data[3].value = 0; // dmRes.period_num
	data[3].parray = NULL;
	strcpy(data[4].name , "region_list");
	data[4].type = 3;
	data[4].array_len = 4; 
	data[4].value = dmRes.region_num; 
	data[4].parray = NULL;

	struct hd_cmd_dict *plist = calloc(7*3,sizeof(struct hd_cmd_dict));
	if(plist != NULL)
	{
	    int k = 0;
	    for(i=0;i<7;i++)
	    {
		//printf("dmRes.period_list[%d].week = %d,start:%s,end:%s\n",i,
		//	dmRes.period_list[i].week,dmRes.period_list[i].start,dmRes.period_list[i].end);
		if(dmRes.period_list[i].week == i+1)
		{
		    strcpy(plist[k*3+0].name,"week");
		    plist[k*3+0].type = 0;
		    plist[k*3+0].value= dmRes.period_list[i].week;
		    strcpy(plist[k*3+1].name,"start");
		    plist[k*3+1].type = 1;
		    strcpy(plist[k*3+1].string,dmRes.period_list[i].start);
		    strcpy(plist[k*3+2].name,"end");
		    plist[k*3+2].type = 1;
		    strcpy(plist[k*3+2].string,dmRes.period_list[i].end);

		    data[3].value = ++k;
		}
	    }
	    if(data[3].value > 0) data[3].parray = plist;
	    else { free(plist); plist = NULL;}

	    if(dmRes.region_num > 0)
	    {
		plist = calloc(dmRes.region_num*4,sizeof(struct hd_cmd_dict));
		if(plist != NULL)
		{
		    int k = 0;
		    data[4].parray = plist;
		    for(i=0;i<dmRes.region_num;i++)
		    {
			strcpy(plist[k*4+0].name,"left");
			plist[k*4+0].type = 0;
			plist[k*4+0].value= dmRes.region_list[i].left;
			strcpy(plist[k*4+1].name,"top");
			plist[k*4+1].type = 0;
			plist[k*4+1].value= dmRes.region_list[i].top;
			strcpy(plist[k*4+2].name,"right");
			plist[k*4+2].type = 0;
			plist[k*4+2].value= dmRes.region_list[i].right;
			strcpy(plist[k*4+3].name,"bottom");
			plist[k*4+3].type = 0;
			plist[k*4+3].value= dmRes.region_list[i].bottom;
		    }
		}
	    }
	    preq = hd_build_ack("get_motion_detect_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,data,5);
	}
	else
	{
	    preq = hd_build_ack("get_motion_detect_ack",hdh->last_serial,E_DEV_ERROR_INTERNAL,"malloc fail",0,0);
	}
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

/** type: motion_detect_alarm移动侦测，region_boundary_alarm越界侦测,region_intrude_alarm闯入侦测 */
int hd_notify_alarm_event(HD_DEV_HANDLE hdh,struct dm_notify_alarm_msg *msg)
{
    char *preq = NULL;
    int ret = 0,dnum = 9;
    struct hd_cmd_dict data[9];

    if(hdh==NULL || msg==NULL) 
    {
	return -1;
    }
    else if(hdh->login_mgr_ok==0)
    {
	return -2;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC,&now);
    if(strcmp(msg->type,hdh->last_alarm_type)==0 && now.tv_sec - hdh->last_alarm_ts.tv_sec <= 5)
    {
	return -3;
    }
    strcpy(hdh->last_alarm_type,msg->type);
    hdh->last_alarm_ts = now;

    strcpy(data[0].name , "type");
    data[0].type = 1;
    strcpy(data[0].string,msg->type);

    strcpy(data[1].name , "time");
    data[1].type = 0;
    data[1].value = 0;

    strcpy(data[2].name , "channel");
    data[2].type = 0;
    data[2].value = 0;

    if(strlen(msg->image_name)>0 && msg->image_size > 0)
    {
	strcpy(data[3].name , "image_name");
	data[3].type = 1;
	strcpy(data[3].string,msg->image_name);

	strcpy(data[4].name , "image_size");
	data[4].type = 0;
	data[4].value = msg->image_size;

	strcpy(data[5].name , "cloud_storage_path");
	data[5].type = 1;
	strcpy(data[5].string,msg->cloud_storage_path);

	strcpy(data[6].name , "cloud_storage_type");
	data[6].type = 0;
	data[6].value = msg->cloud_storage_type;

	strcpy(data[7].name , "upload_start_time");
	data[7].type = 0;
	data[7].value = msg->upload_start_time;

	strcpy(data[8].name , "upload_end_time");
	data[8].type = 0;
	data[8].value = msg->upload_end_time;
    }
    else
    {
	strcpy(data[3].name , "image_name");
	data[3].type = 1;
	strcpy(data[3].string,"");

	strcpy(data[4].name , "image_size");
	data[4].type = 0;
	data[4].value = 10;

	strcpy(data[5].name , "cloud_storage_path");
	data[5].type = 1;
	strcpy(data[5].string,"");

	strcpy(data[6].name , "cloud_storage_type");
	data[6].type = 0;
	data[6].value = 1;

	strcpy(data[7].name , "upload_start_time");
	data[7].type = 0;
	data[7].value = 0;

	strcpy(data[8].name , "upload_end_time");
	data[8].type = 0;
	data[8].value = 0;
    }

    preq = hd_build_ack("device_alarm_report",hdh->last_serial,msg->ack.error,msg->ack.reason,data,dnum);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) 
	{
	    ret = -4;
	}
	else
	{
	    ret = 0;
	}
	free(preq);
    }
    return ret;
}

int hd_notify_download_start(HD_DEV_HANDLE hdh,int sock, const char *token)
{
    struct hd_cmd_dict data[1];
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL || sock <= 0)
	return -1;

    strcpy(data[0].name , "token");
    data[0].type = 1;
    strcpy(data[0].string,token);
    preq = hd_build_ack("download_start_notify",hdh->last_serial,0,"ok",data,1);
    if(preq!=NULL)
    {
	ret = send(sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) ret = -1;
	free(preq);
    }
    return ret;
}
int hd_notify_download_keepalive(HD_DEV_HANDLE hdh,int sock)
{
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL || sock <= 0)
	return -1;

    preq = hd_build_ack_just_ok("download_keepalive_notify",hdh->last_serial);
    if(preq!=NULL)
    {
	ret = send(sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) ret = -1;
	free(preq);
    }
    return ret;
}
int hd_notify_download_finish(HD_DEV_HANDLE hdh,int sock)
{
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL || sock <= 0)
	return -1;

    preq = hd_build_ack_just_ok("download_finish_notify",hdh->last_serial);
    if(preq!=NULL)
    {
	ret = send(sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) ret = -1;
	free(preq);
    }
    return ret;
}

int hd_req_lens_init(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_lens_init)
    {
	ret = hd_res_operation_unsupport(hdh,"lens_init_ack");
	return ret;
    }
    struct dm_lens_init_req dmReq ;
    struct dm_lens_init_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    dm_cbfuncs.cmd_req_lens_init(&dmReq,&dmRes);

    preq = hd_build_ack("lens_init_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	free(preq);
	return ret;
    }
    return -1;
}

int hd_req_focus_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_focus_ctrl)
    {
	ret = hd_res_operation_unsupport(hdh,"focus_ctrl_ack");
	return ret;
    }
    struct dm_focus_ctrl_req dmReq ;
    struct dm_focus_ctrl_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"focus");
	dmReq.focus = pitem->valueint;

	dm_cbfuncs.cmd_req_focus_ctrl(&dmReq,&dmRes);

	preq = hd_build_ack("focus_ctrl_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_zoom_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_zoom_ctrl)
    {
	ret = hd_res_operation_unsupport(hdh,"zoom_ctrl_ack");
	return ret;
    }
    struct dm_zoom_ctrl_req dmReq ;
    struct dm_zoom_ctrl_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"zoom");
	dmReq.zoom = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"speed");
	dmReq.speed = pitem->valueint;

	dm_cbfuncs.cmd_req_zoom_ctrl(&dmReq,&dmRes);

	preq = hd_build_ack("zoom_ctrl_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_aperture_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_aperture_ctrl)
    {
	ret = hd_res_operation_unsupport(hdh,"aperture_ctrl_ack");
	return ret;
    }
    struct dm_aperture_ctrl_req dmReq ;
    struct dm_aperture_ctrl_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"aperture");
	dmReq.aperture = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"speed");
	dmReq.speed = pitem->valueint;

	dm_cbfuncs.cmd_req_aperture_ctrl(&dmReq,&dmRes);

	preq = hd_build_ack("aperture_ctrl_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_set_preset_point(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_set_preset_point)
    {
	ret = hd_res_operation_unsupport(hdh,"set_preset_point_ack");
	return ret;
    }
    struct dm_set_preset_point_req dmReq ;
    struct dm_set_preset_point_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"number");
	dmReq.number = pitem->valueint;

	dm_cbfuncs.cmd_req_set_preset_point(&dmReq,&dmRes);

	preq = hd_build_ack("set_preset_point_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}
int hd_req_del_preset_point(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_del_preset_point)
    {
	ret = hd_res_operation_unsupport(hdh,"del_preset_point_ack");
	return ret;
    }
    struct dm_del_preset_point_req dmReq ;
    struct dm_del_preset_point_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"number");
	dmReq.number = pitem->valueint;

	dm_cbfuncs.cmd_req_del_preset_point(&dmReq,&dmRes);

	preq = hd_build_ack("del_preset_point_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_set_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;
    int num = 0;

    if(NULL == dm_cbfuncs.cmd_req_set_cruise)
    {
	ret = hd_res_operation_unsupport(hdh,"set_cruise_ack");
	return ret;
    }
    struct dm_set_cruise_req dmReq ;
    struct dm_set_cruise_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"cruise_name");
	strcpy(dmReq.cruise_name,pitem->valuestring);
	pitem = ayJSON_GetObjectItem(pdata,"preset_point_list");
	if(pitem!=NULL && (num=ayJSON_GetArraySize(pitem))>0)
	{
	    int i = 0;
	    for(i=0;i<num;i++)
	    {
		ayJSON *punit = ayJSON_GetArrayItem(pitem,i);
		dmReq.preset_point_list[i] = punit->valueint;
	    }
	}

	dm_cbfuncs.cmd_req_set_cruise(&dmReq,&dmRes);

	preq = hd_build_ack("set_cruise_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_del_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_del_cruise)
    {
	ret = hd_res_operation_unsupport(hdh,"del_cruise_ack");
	return ret;
    }
    struct dm_del_cruise_req dmReq ;
    struct dm_del_cruise_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"cruise_name");
	strcpy(dmReq.cruise_name,pitem->valuestring);

	dm_cbfuncs.cmd_req_del_cruise(&dmReq,&dmRes);

	preq = hd_build_ack("del_cruise_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_start_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_start_cruise)
    {
	ret = hd_res_operation_unsupport(hdh,"start_cruise_ack");
	return ret;
    }
    struct dm_start_cruise_req dmReq ;
    struct dm_start_cruise_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pdata,"cruise_name");
	strcpy(dmReq.cruise_name,pitem->valuestring);

	dm_cbfuncs.cmd_req_start_cruise(&dmReq,&dmRes);

	preq = hd_build_ack("start_cruise_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}

int hd_req_stop_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata)
{
    char *preq = NULL;
    int ret = 0;

    if(NULL == dm_cbfuncs.cmd_req_stop_cruise)
    {
	ret = hd_res_operation_unsupport(hdh,"stop_cruise_ack");
	return ret;
    }
    struct dm_stop_cruise_req dmReq ;
    struct dm_stop_cruise_ack dmRes ;

    memset((void*)&dmReq,0,sizeof(dmReq));
    memset((void*)&dmRes,0,sizeof(dmRes));
    ayJSON *pitem = ayJSON_GetObjectItem(pdata,"channel");
    if(pitem!=NULL)
    {
	dmReq.channel = pitem->valueint;

	dm_cbfuncs.cmd_req_stop_cruise(&dmReq,&dmRes);

	preq = hd_build_ack("stop_cruise_ack",hdh->last_serial,dmRes.ack.error,dmRes.ack.reason,0,0);
	if(preq!=NULL)
	{
	    ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	    free(preq);
	    return ret;
	}
	return -1;
    }
    return -1;
}
