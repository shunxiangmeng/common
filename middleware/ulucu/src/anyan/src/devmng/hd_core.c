#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"
#include "define.h"
#include "ayutil.h"
#include "hd_core.h"
#include "hd_adapter.h"
#include "AnyanDeviceMng.h"
#include "Anyan_Device_SDK.h"

static HD_DEV_HANDLE gs_hdh = NULL;
static pthread_t gs_dm_tid;
static int gs_dm_runflag = 0;

int ADM_Start_Client(void)
{
    if(gs_hdh)
    {
	int ret = 0;
	gs_dm_runflag = 1;
	ret = pthread_create(&gs_dm_tid,NULL,hd_process_thread,NULL);
	if(ret != 0)
	{
	    DEBUGF("create dm thread fail!,ret = %d\n",ret);
	    return -1;
	}
	return 0;
    }
    return -1;
}

void ADM_Stop_Client(void)
{
    if(gs_hdh)
    {
	gs_dm_runflag = 0;
	pthread_join(gs_dm_tid,NULL);
    }
}

int ADM_Init_Env(struct dm_cmd_callback cbfuncs)
{
    ADM_Set_CmdCallback(cbfuncs);

    HD_DEV_HANDLE hdh = NULL;

    if(gs_hdh==NULL)
    {
	hdh = malloc(sizeof(struct hd_dev_instance));
	if(hdh == NULL)
	    return -1;
	memset(hdh,0,sizeof(*hdh));

	strcpy(hdh->blserver_name,HD_BLSERVER_NAME);
	strcpy(hdh->last_serial,"anyandevice");
	hdh->blserver_port = HD_BLSERVER_PORT;
	hdh->bl_sock = -1;
	hdh->mgr_sock = -1;
	hdh->login_mgr_ok = 0;
	clock_gettime(CLOCK_MONOTONIC,&hdh->last_alarm_ts);

	gs_hdh = hdh;
	return ADM_Start_Client();
    }
    return -1;
}

void ADM_Exit_Env(void)
{
    ADM_Stop_Client();

    if(gs_hdh!=NULL)
    {
	HD_DEV_HANDLE hdh = gs_hdh;

	if(hdh->bl_sock>0)
	    close(hdh->bl_sock);
	if(hdh->mgr_sock>0)
	    close(hdh->mgr_sock);
	memset(hdh,0,sizeof(*hdh));
	free(hdh);
	gs_hdh = NULL;
    }
}

ayJSON *build_data_json(struct hd_cmd_dict values[],int num)
{
    int i = 0;
    ayJSON *item = NULL;
    ayJSON *pdata = ayJSON_CreateObject();
    for(i=0;i<num;i++)
    {
	if(values[i].type==1)
	{
	    item = ayJSON_CreateString(values[i].string);
	    ayJSON_AddItemToObject(pdata,values[i].name,item); 
	}
	else if(values[i].type==0)
	{
	    item = ayJSON_CreateNumber(values[i].value);
	    ayJSON_AddItemToObject(pdata,values[i].name,item); 
	}
	else if(values[i].type==3)
	{
	   if(values[i].parray!=NULL && values[i].array_len>0)
	   {
	       int k = 0;
	       item = ayJSON_CreateArray();
	       for(k=0;k<values[i].value;k++)
	       {
		   ayJSON_AddItemToArray(item,build_data_json(values[i].parray + k*values[i].array_len, values[i].array_len));
	       }
	       ayJSON_AddItemToObject(pdata,values[i].name,item); 
	   }
	}
    }
    return pdata;
}

/* Need the caller to free the memory */
char *hd_build_req(const char *cmd,char *serial,struct hd_cmd_dict values[],int num)
{
    ayJSON *pjson = ayJSON_CreateObject();
    ayJSON *item = NULL;

    if(pjson == NULL)
	return NULL;

    item = ayJSON_CreateString(serial);
    ayJSON_AddItemToObject(pjson,"serial",item); 
    item = ayJSON_CreateString(cmd);
    ayJSON_AddItemToObject(pjson,"cmd",item); 

    ayJSON *pdata = build_data_json(values,num);
    ayJSON_AddItemToObject(pjson,"data",pdata); 
    
    char *json_str = ayJSON_PrintUnformatted(pjson);
    char *json_req = NULL;
    if(json_str!=NULL)
    {
	DEBUGF("[%ld]req json:%s\n",time(NULL),json_str);
	json_req = malloc(strlen(json_str)+8);
	if(json_req!=NULL)
	{
	    sprintf(json_req,"%s\r\n\r\n",json_str);
	}
	free(json_str);
    }
    ayJSON_Delete(pjson);
    return json_req;
}

/* Need the caller to free the memory */
char *hd_build_ack(const char *cmd,char *serial,int errcode,const char *errstr, struct hd_cmd_dict values[],int num)
{
    ayJSON *pjson = ayJSON_CreateObject();
    ayJSON *item = NULL;

    if(pjson == NULL)
	return NULL;

    item = ayJSON_CreateString(serial);
    ayJSON_AddItemToObject(pjson,"serial",item); 

    item = ayJSON_CreateString(cmd);
    ayJSON_AddItemToObject(pjson,"cmd",item); 
    item = ayJSON_CreateNumber(errcode);
    ayJSON_AddItemToObject(pjson,"error",item); 
    item = ayJSON_CreateString(errstr);
    ayJSON_AddItemToObject(pjson,"reason",item); 

    ayJSON *pdata = build_data_json(values,num);
    ayJSON_AddItemToObject(pjson,"data",pdata); 

    char *json_str = ayJSON_PrintUnformatted(pjson);
    char *json_ack = NULL;
    if(json_str!=NULL)
    {
	DEBUGF("[%ld]ack json:%s\n",time(NULL),json_str);
	json_ack = malloc(strlen(json_str)+8);
	if(json_ack!=NULL)
	{
	    sprintf(json_ack,"%s\r\n\r\n",json_str);
	}
	free(json_str);
    }
    ayJSON_Delete(pjson);
    return json_ack;
}

char *hd_build_ack_just_ok(const char *cmd,char *serial)
{
    return hd_build_ack(cmd,serial,0,"ok",0,0);
}

static int hd_req_device_login_ldb(HD_DEV_HANDLE hdh,const char *sn)
{
    struct hd_cmd_dict data[2];
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL || sn==NULL)
	return -1;
    if(hdh->bl_sock <= 0)
    {
	hdh->bl_sock = ayutil_tcp_client(hdh->blserver_name,hdh->blserver_port,3);
	if(hdh->bl_sock < 0)
	{
	    DEBUGF("connect blserver[%s:%d] fail!\n",hdh->blserver_name,hdh->blserver_port);
	    return -1;
	}
    }
    snprintf(hdh->devid,63,"%s",sn);
    memset(&data,0,sizeof(data));
    strcpy(data[0].name , "sn");
    data[0].type = 1;
    strcpy(data[0].string,hdh->devid);

    strcpy(data[1].name , "protocol_version");
    data[1].type = 1;
    strcpy(data[1].string,HD_PROTOCOL_VER);

    preq = hd_build_req("device_login_ldb_req",hdh->last_serial,data,2);
    if(preq!=NULL)
    {
	printf("[ulk login ldb req]: %s\n",preq);
	ret = send(hdh->bl_sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret < 0)
	{
	    DEBUGF("send ldb req fail:%s\n",strerror(ret));
	    close(hdh->bl_sock);
	    hdh->bl_sock = -1;
	}
	free(preq);
    }
    return ret;
}

static int hd_req_device_login_mgr(HD_DEV_HANDLE hdh)
{
    struct hd_cmd_dict data[1];
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL)
	return -1;
    if(hdh->mgr_sock <= 0)
    {
	hdh->mgr_sock = ayutil_tcp_client(hdh->devmgr_server_name,hdh->devmgr_server_port,3);
	if(hdh->mgr_sock < 0)
	{
	    DEBUGF("connect blserver[%s:%d] fail!\n",hdh->devmgr_server_name,hdh->devmgr_server_port);
	    return -1;
	}
    }
    memset(&data,0,sizeof(data));
    strcpy(data[0].name , "token");
    data[0].type = 1;
    strcpy(data[0].string,hdh->token);

    preq = hd_build_req("device_login_mgr_req",hdh->last_serial,data,1);
    if(preq!=NULL)
    {
	printf("[ulk login mgr req]: %s\n",preq);
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) 
	{
	    close(hdh->mgr_sock);
	    hdh->mgr_sock = -1;
	    ret = -1;
	}
	free(preq);
    }
    return ret;
}

static int hd_req_device_keepalive_mgr(HD_DEV_HANDLE hdh)
{
    char *preq = NULL;
    int ret = 0;

    if(hdh==NULL || hdh->mgr_sock <= 0)
	return -1;

    preq = hd_build_req("device_keepalive_mgr_req",hdh->last_serial,0,0);
    if(preq!=NULL)
    {
	ret = send(hdh->mgr_sock,preq,strlen(preq),MSG_NOSIGNAL);
	if(ret != strlen(preq)) ret = -1;
	free(preq);
    }
    return ret;
}

static int hd_handle_cmd(HD_DEV_HANDLE hdh, const char *json_cmd, const char *wait_cmd)
{
    int ret = 0;
    int errcode = 0;
    char cmd[128]={0};
    char errstr[1024]={0};

    printf("ulk hd json cmd[t:%ld]:%s",time(NULL),json_cmd);
    ayJSON *pjson = ayJSON_Parse(json_cmd);
    if(pjson==NULL) 
	return -1;
    ayJSON *pitem = ayJSON_GetObjectItem(pjson,"cmd");
    if(pitem == NULL || strlen(pitem->valuestring)==0) 
    {
	ayJSON_Delete(pjson);
	return -1;
    }
    strcpy(cmd,pitem->valuestring);
    if(wait_cmd!=NULL && strcmp(cmd,wait_cmd))
    {
	DEBUGF("waitcmd:%s, but cmd:%s isnot!\n",wait_cmd,cmd);
	ayJSON_Delete(pjson);
	return -1;
    }
    pitem = ayJSON_GetObjectItem(pjson,"serial");
    if(pitem != NULL)
    {
	memset(hdh->last_serial,0,sizeof(hdh->last_serial));
	strncpy(hdh->last_serial,pitem->valuestring,63);
    }
    /* only ack has */
    pitem = ayJSON_GetObjectItem(pjson,"error");
    if(pitem!=NULL)
    {
	hdh->last_errcode = errcode = pitem->valueint;
	pitem = ayJSON_GetObjectItem(pjson,"reason");
	if(pitem!=NULL) 
	{
	    strcpy(errstr,pitem->valuestring);
	}
	if(errcode!=0)
	{
	    DEBUGF("ack cmd err[%d:%s]!!!\n",errcode,errstr);
	    ayJSON_Delete(pjson);
	    return -1;
	}
    }
    //printf("----cmd:%s,serial:%s\n",cmd,hdh->last_serial);
    pitem = ayJSON_GetObjectItem(pjson,"data");
    if(pitem!=NULL)
    {
	if(strcmp(cmd,"device_login_ldb_ack")==0)
	{
	    ayJSON *pdata = ayJSON_GetObjectItem(pitem,"token");
	    if(pdata!=NULL)
	    {
		strncpy(hdh->token,pdata->valuestring,255);
	    }
	    pdata = ayJSON_GetObjectItem(pitem,"devmgr");
	    if(pdata!=NULL)
	    {
		sscanf(pdata->valuestring,"%[0-9.]:%hu",hdh->devmgr_server_name,&hdh->devmgr_server_port);
	    }
	    ret = 0;
	}
	else if(strcmp(cmd,"device_login_mgr_ack")==0)
	{
	    ret = 0; 
	}
	else if(strcmp(cmd,"device_keepalive_mgr_ack")==0)
	{
	    ret = 0;
	}
	/* Upgrade Management Commands */
	else if(strcmp(cmd,"get_device_version_req")==0)
	{
	    ret = hd_req_get_device_version(hdh,pitem);
	}
	else if(strcmp(cmd,"start_update_req")==0)
	{
	    ret = hd_req_start_update(hdh,pitem);
	}
	/* Other Control Commands */
	else if(strcmp(cmd,"start_screenshot_req")==0)
	{
	    ret = hd_req_start_screenshot(hdh,pitem);
	}
	/* Record Management Commands */
	else if(strcmp(cmd,"query_record_req")==0)
	{
	    ret = hd_req_query_record(hdh,pitem);
	}
	else if(strcmp(cmd,"start_download_req")==0)
	{
	    ret = hd_req_start_download(hdh,pitem);
	}
	else if(strcmp(cmd,"stop_download_req")==0)
	{
	    ret = hd_req_stop_download(hdh,pitem);
	}
	/********** PTZ Commands *******************/
	else if(strcmp(cmd,"lens_init_req")==0)
	{
	    ret = hd_req_lens_init(hdh,pitem);
	}
	else if(strcmp(cmd,"direction_ctrl_req")==0)
	{
	    ret = hd_req_direction_ctrl(hdh,pitem);
	}
	else if(strcmp(cmd,"stop_ctrl_req")==0)
	{
	    ret = hd_req_stop_ptzctrl(hdh,pitem);
	}
	else if(strcmp(cmd,"focus_ctrl_req")==0)
	{
	    ret = hd_req_focus_ctrl(hdh,pitem);
	}
	else if(strcmp(cmd,"zoom_ctrl_req")==0)
	{
	    ret = hd_req_zoom_ctrl(hdh,pitem);
	}
	else if(strcmp(cmd,"aperture_ctrl_req")==0)
	{
	    ret = hd_req_aperture_ctrl(hdh,pitem);
	}
	else if(strcmp(cmd,"set_preset_point_req")==0)
	{
	    ret = hd_req_set_preset_point(hdh,pitem);
	}
	else if(strcmp(cmd,"del_preset_point_req")==0)
	{
	    ret = hd_req_del_preset_point(hdh,pitem);
	}
	else if(strcmp(cmd,"set_cruise_req")==0)
	{
	    ret = hd_req_set_cruise(hdh,pitem);
	}
	else if(strcmp(cmd,"del_cruise_req")==0)
	{
	    ret = hd_req_del_cruise(hdh,pitem);
	}
	else if(strcmp(cmd,"start_cruise_req")==0)
	{
	    ret = hd_req_start_cruise(hdh,pitem);
	}
	else if(strcmp(cmd,"set_motion_detect_req")==0)
	{
	    ret = hd_req_set_motion_detect(hdh,pitem);
	}
	else if(strcmp(cmd,"get_motion_detect_req")==0)
	{
	    ret = hd_req_get_motion_detect(hdh,pitem);
	}
	/*************** Log Debug ********************/
	else if(strcmp(cmd,"open_debug_req")==0)
	{
	    ret = hd_req_open_debug(hdh,pitem);
	}
	else if(strcmp(cmd,"close_debug_req")==0)
	{
	    ret = hd_req_close_debug(hdh,pitem);
	}
	else
	{
	    ret = hd_res_operation_unsupport(hdh,cmd);
	}
    }
    ayJSON_Delete(pjson);
    return ret;
}

static void hd_sleep(int secs)
{
    while(gs_dm_runflag && secs--)
	sleep(1);
}

void *hd_process_thread(void *args)
{
    int ret = 0;
    char recvbuf[4096]={0},cmdbuf[4096]={0};
    char *pend = NULL;
    int recvlen = 0,cmdlen = 0,hb_cnt = 0;
    int status = HD_MACH_LOGIN_LDB;
    struct timespec	t1,t2;
    int json_errcode = 0, err_cnt = 0;

    HD_DEV_HANDLE hdh = gs_hdh;

    set_thread_name("hd_process");
    //while(!sdk_get_handle(0)->exit_flag)
    while (gs_dm_runflag)
    {
	//printf("status = %d\n",status);
	switch(status)
	{
	    case HD_MACH_LOGIN_LDB:
            ret = hd_req_device_login_ldb(hdh, Ulu_SDK_Get_Device_ID());
            if (ret > 0)
            {
                status = HD_MACH_LDB_RECV;
            }
            else
            {
                hd_sleep(5);
            }
            break;
	    case HD_MACH_LDB_RECV:
            ret = recv(hdh->bl_sock,recvbuf+recvlen,sizeof(recvbuf)-recvlen,0);
            if(ret > 0)
            {
                //printf("ldb recv ret = %d\n",ret);
                recvlen += ret;
                recvbuf[recvlen] = 0;
                if((pend=strstr(recvbuf,"\r\n\r\n"))!=NULL)
                {
                memset(cmdbuf,0,sizeof(cmdbuf));
                strncpy(cmdbuf,recvbuf,(pend+4)-recvbuf);
                status = HD_MACH_LDB_ACK;
                }
            }
            else
            {
                DEBUGF("ldb recv err,ret[%d]: %s\n",ret,strerror(errno));
                memset(recvbuf,0,sizeof(recvbuf));
                recvlen = 0;
                close(hdh->bl_sock);
                hdh->bl_sock = -1;
                status = HD_MACH_LOGIN_LDB;
                hd_sleep(5);
            }
            break;
	    case HD_MACH_LDB_ACK:
		memset(recvbuf,0,sizeof(recvbuf));
		recvlen = 0;
		ret = hd_handle_cmd(hdh,cmdbuf,"device_login_ldb_ack");	
		if(hdh->last_errcode == 0)
		{
		    json_errcode = err_cnt = 0;
		    close(hdh->bl_sock);
		    hdh->bl_sock = -1;
		    status = HD_MACH_LOGIN_MGR;
		}
		else if(json_errcode != hdh->last_errcode)
		{
		    json_errcode = hdh->last_errcode;
		    err_cnt = 0 ;
		    status = HD_MACH_LOGIN_LDB;
		    hd_sleep(5);
		}
		else
		{
		    err_cnt ++;
		    hd_sleep(err_cnt*5);
		    status = HD_MACH_LOGIN_LDB;
		}
		break;
	    case HD_MACH_LOGIN_MGR:
		memset(recvbuf,0,sizeof(recvbuf));
		recvlen = 0;
		ret = hd_req_device_login_mgr(hdh);
		if(ret >= 0)
		{
		    status = HD_MACH_MGR_RECV;
		    clock_gettime(CLOCK_MONOTONIC,&t1);
		}
		else
		{
		    hd_sleep(5);
		}
		break;
	    case HD_MACH_MGR_RECV:
		clock_gettime(CLOCK_MONOTONIC,&t2);
		if(t2.tv_sec - t1.tv_sec > HD_HEART_INTERVAL)
		{
		    status = HD_MACH_MGR_BEAT;
		}
		recvbuf[recvlen] = 0;
		if((pend=strstr(recvbuf,"\r\n\r\n"))!=NULL)
		{
		    DEBUGF("recvlen = %d\n",recvlen);
		    cmdlen = pend+4 - recvbuf;;
		    memset(cmdbuf,0,sizeof(cmdbuf));
		    strncpy(cmdbuf,recvbuf,cmdlen);
		    recvlen -= cmdlen;
		    if(recvlen > 0) memmove(recvbuf,pend+4,recvlen);
		    status = HD_MACH_MGR_CMD;
		}
		else
		{
		    //printf(">>>>>>[%ld] mgr recv\n",time(NULL));
		    ret = recv(hdh->mgr_sock,recvbuf+recvlen,sizeof(recvbuf)-recvlen,0);
		    if(ret > 0)
		    {
			//printf("[%ld] mgr recv ret = %d\n",time(NULL),ret);
			recvlen += ret;
			recvbuf[recvlen] = 0;
			if((pend=strstr(recvbuf,"\r\n\r\n"))!=NULL)
			{
			    cmdlen = pend+4 - recvbuf;
			    memset(cmdbuf,0,sizeof(cmdbuf));
			    strncpy(cmdbuf,recvbuf,cmdlen);
			    recvlen -= cmdlen;
			    if(recvlen > 0) memmove(recvbuf,pend+4,recvlen);
			    if(hdh->login_mgr_ok == 0) 
				status = HD_MACH_LOGIN_MGR_ACK;
			    else 
				status = HD_MACH_MGR_CMD;
			}
		    }
		    else if((errno!=EAGAIN && errno!=EWOULDBLOCK) || hb_cnt > 3)
		    {
			DEBUGF("mgr recv err,ret[%d]: %s\n",ret,strerror(errno));
			memset(recvbuf,0,sizeof(recvbuf));
			recvlen = hb_cnt = 0;
			close(hdh->mgr_sock);
			hdh->mgr_sock = -1;
			hdh->login_mgr_ok = 0;
			status = HD_MACH_LOGIN_MGR;
		    }
		}
		break;
	    case HD_MACH_LOGIN_MGR_ACK:
		memset(recvbuf,0,sizeof(recvbuf));
		recvlen = 0;
		ret = hd_handle_cmd(hdh,cmdbuf,"device_login_mgr_ack");	
		if(hdh->last_errcode == 0)
		{
		    hdh->login_mgr_ok = 1;
		    json_errcode = err_cnt = hb_cnt = 0;
		    status = HD_MACH_MGR_RECV;
		}
		else if(json_errcode != hdh->last_errcode)
		{
		    json_errcode = hdh->last_errcode;
		    err_cnt = 0 ;
		    status = HD_MACH_LOGIN_MGR;
		    hd_sleep(5);
		}
		else
		{
		    err_cnt ++;
		    if(err_cnt < 10)
		    {
			hd_sleep(err_cnt*5);
			status = HD_MACH_LOGIN_MGR;
		    }
		    else
		    {
			err_cnt = 0;
			status = HD_MACH_LOGIN_LDB;
		    }
		}
		break;
	    case HD_MACH_MGR_CMD:
		ret = hd_handle_cmd(hdh,cmdbuf,NULL);	
		hb_cnt = 0;
		status = HD_MACH_MGR_RECV;
		cmdlen = 0;
		memset(cmdbuf,0,sizeof(cmdbuf));
		break;
	    case HD_MACH_MGR_BEAT:
		clock_gettime(CLOCK_MONOTONIC,&t1);
		ret = hd_req_device_keepalive_mgr(hdh);
		if(ret >= 0)
		{
		    status = HD_MACH_MGR_RECV;
		    hb_cnt ++;
		}
		else if(hdh->mgr_sock >= 0)
		{
		    close(hdh->mgr_sock);
		    hdh->mgr_sock = -1;
		    status = HD_MACH_LOGIN_MGR;
		}
		break;
	}
    }

    return NULL;
}

int ADM_notify_download_start(int sock, const char *token)
{
    return hd_notify_download_start(gs_hdh,sock,token);
}

int ADM_notify_download_keepalive(int sock)
{
    return hd_notify_download_keepalive(gs_hdh,sock);
}

int ADM_notify_download_finish(int sock)
{
    return hd_notify_download_finish(gs_hdh,sock);
}

int ADM_notify_alarm_event(struct dm_notify_alarm_msg *msg)
{
    return hd_notify_alarm_event(gs_hdh,msg);
}

