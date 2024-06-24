#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "ayqueue.h"
#include "ayutil.h"
#include "aystream.h"
#include "aydevice2.h"
#include "ayinit.h"
#include "ay_sdk.h"

#include "monitor.h"
#include "threadpool.h"
#include "watchdog.h"
#include "protocol_device.h"
#include "version.h"
#include "infra/include/Logger_c.h"

ay_sdk_handle ay_psdk = NULL;

static int Get_Alarm_Permission(ULK_Alarm_Struct *pctrl, struct timespec last_alarm_time)
{
    struct tm  *local, curtm;
    uint32  time_tmp;
    struct timespec now;

    if (pctrl->alarm_flag == 0) {
        errorf("alarm is forbid\n");
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC,&now);
    if (ayutil_cost_mseconds(last_alarm_time,now) <= pctrl->alarm_interval_mix * 1000) {
        DEBUGF("alarm too fast,interval mix = %d\n",pctrl->alarm_interval_mix);
        return  -3;
    }

    time_t t = time(NULL); 
    local = localtime_r(&t,&curtm); 

    time_tmp = local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec;
    DEBUGF("localtime time = %u\n", time_tmp);
    for (int i = 0 ; i < pctrl->alarm_time_table_num; i++) {
        if (time_tmp <= pctrl->alarm_time_table[i].end && time_tmp >= pctrl->alarm_time_table[i].start) {
            return 0;
        }
    }
    DEBUGF("alarm time is not allow\n");
    return -2;
}

ay_sdk_handle sdk_init_instace(int id)
{
    if (ay_psdk == NULL) {
        ay_psdk = (ay_sdk_handle)malloc(sizeof(st_ay_sdk_instance));
        if (ay_psdk != NULL) {
            memset(ay_psdk, 0, sizeof(st_ay_sdk_instance));
            pthread_mutex_init(&ay_psdk->frame_report_lock, NULL);
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
            pthread_mutex_init(&ay_psdk->stream_flag_lock, &attr);
            pthread_mutexattr_destroy(&attr);
        }
    }
    return ay_psdk;
}

ay_sdk_handle sdk_get_handle(int id)
{
    return ay_psdk;
}

void notify_alarm_config_update(ULK_Alarm_Struct *palarm,int num)
{
    CMD_PARAM_STRUCT	cmd;
    AY_Alarm_Struct	ay_alarm;

    cmd.channel = 0;
    cmd.cmd_id = ALARM_CTRL;
    cmd.cmd_args_len = num*sizeof(AY_Alarm_Struct);
    memset(cmd.cmd_args, 0, sizeof(cmd.cmd_args));

    for (int i = 0; i < num; i++) {
        memset(&ay_alarm, 0, sizeof(ay_alarm));
        ay_alarm.alarm_flag = palarm[i].alarm_flag;
        ay_alarm.alarm_time_table_num = palarm[i].alarm_time_table_num;
        ay_alarm.alarm_interval_mix = palarm[i].alarm_interval_mix;
        for (int j = 0; j < ay_alarm.alarm_time_table_num; j++) {
            ay_alarm.alarm_time_table[j].start = palarm[i].alarm_time_table[j].start;
            ay_alarm.alarm_time_table[j].end = palarm[i].alarm_time_table[j].end;
        }
        memcpy(cmd.cmd_args + i * sizeof(AY_Alarm_Struct), &ay_alarm, sizeof(AY_Alarm_Struct));
    }

    Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
}

/* msg 最大1024个字符 */
void call_back_error_info(char *msg)
{
    CMD_PARAM_STRUCT err;
    err.channel = 0;
    err.cmd_id = ERROR_INFO;
    int len = strlen(msg);
    memset(err.cmd_args, 0, sizeof(err.cmd_args));

    if (len >= sizeof(err.cmd_args) - 1)
    {
        memcpy(err.cmd_args, msg, sizeof(err.cmd_args) - 1);			
    }else{
        strcpy(err.cmd_args, msg);
    }
    Msg_Cmd_Add_down_queue((char*)&err, sizeof(CMD_PARAM_STRUCT));
}
 
int Computer_Net_UploadSpeed(uint32 tlen)
{
    if (tlen > 0) {
        struct net_statistic_info netinfo;
        char  buf[256]={0};

        trace_get_net_stat_info(&netinfo);
        snprintf(buf, 256, "need %7.2fkB/s,real %7.2fkB/s,stream %7.2fKB/s\n", 
            netinfo.want_send_speed[1], netinfo.real_send_speed[1], sdk_get_handle(0)->net_stream_upload_bytes*1.0 / (1024 * tlen));
        DEBUGF("%s", buf);

        sdk_get_handle(0)->net_stream_upload_bytes = 0;
        call_back_error_info(buf);
    }
    return 0;
}

int ay_init_device_object(st_ay_device_object *pdevobj,int nums,int upnums,int downnums,int talknums)
{
    int ret = 0;

    pdevobj->handle = -1;
    pdevobj->status = 0;
    pdevobj->maxfd = 0;
    FD_ZERO(&pdevobj->rdset);
    FD_ZERO(&pdevobj->wrset);
    pdevobj->pclients = NULL;
    memset(&pdevobj->streamFilter, 0, sizeof(pdevobj->streamFilter));

    ret = ay_init_frm_buf(&pdevobj->streamBuf,nums);
    ret |= init_msg_queue(&pdevobj->cmdUpQueue,upnums);
    ret |= init_msg_queue(&pdevobj->cmdDownQueue,downnums);
    ret |= init_msg_queue(&pdevobj->talkQueue,talknums);
    return ret;
}

int ay_deinit_device_object(st_ay_device_object *pdevobj)
{
    ay_destroy_frm_buf(&pdevobj->streamBuf);
    destroy_msg_queue(&pdevobj->cmdUpQueue);
    destroy_msg_queue(&pdevobj->cmdDownQueue);
    destroy_msg_queue(&pdevobj->talkQueue);
    return 0;
}

static void ay_enable_debug(const char *log_file_full_name)
{
    if (sdk_init_instace(0) && log_file_full_name) {
        ay_sdk_handle psdk = ay_psdk;
        snprintf(psdk->debug.pathfile, sizeof(psdk->debug.pathfile), "%s", log_file_full_name);
        psdk->debug.en_flag = 1;	
        psdk->debug.max_fsize = 50 * 1024;
    }
}

void Ulu_SDK_Enable_Debug(const char *log_file_full_name)
{
    ay_enable_debug(log_file_full_name);
}

int Ulu_SDK_ChannelStatus_Event(int channel, ULK_CHNL_STATUS_ENUM status)
{
    int t = (channel-1) / 8;
    int j = (channel-1) % 8;
    if (sdk_init_instace(0)) {
        uint8 mask = ay_psdk->devinfo.channel_mask[t]&(1<<(7-j));
        if ( (status==ULK_OFFLINE) && mask )
        {// online -> offline
            ay_psdk->devinfo.channel_mask[t] &= ~(1<<(7-j));
            ay_psdk->devinfo.devstatus_update_flag = 1;
        }
        else if((status==ULK_ONLINE) && !mask)
        {// offline -> online
            ay_psdk->devinfo.channel_mask[t] |= 1<<(7-j);
            ay_psdk->devinfo.devstatus_update_flag = 1;
        }
    }
    return 0;
}

static void ay_set_oeminfo(Dev_SN_Info  *Oem_info)
{
    if (sdk_init_instace(0))
    {
		int len , ret;
		memset(&ay_psdk->devinfo.Sn_info, 0 ,sizeof(ay_psdk->devinfo.Sn_info));

		len = sizeof(ay_psdk->devinfo.Sn_info.MAC);
		ret = snprintf(ay_psdk->devinfo.Sn_info.MAC,len,"%s",Oem_info->MAC);
		if (ret > 0 && ret < len)
		{
			if ('\n' == ay_psdk->devinfo.Sn_info.MAC[ret - 1]) 
				ay_psdk->devinfo.Sn_info.MAC[ret - 1] = '\0';
		}
		ay_psdk->devinfo.Sn_info.OEMID = Oem_info->OEMID;

		len = sizeof(ay_psdk->devinfo.Sn_info.SN);
		ret = snprintf(ay_psdk->devinfo.Sn_info.SN,len, "%s", Oem_info->SN);
		if (ret > 0 && ret < len)
		{
			if('\n' == ay_psdk->devinfo.Sn_info.SN[ret-1]) 
				ay_psdk->devinfo.Sn_info.SN[ret-1] = '\0';
		}

		len = sizeof(ay_psdk->devinfo.Sn_info.OEM_name);
		ret = snprintf(ay_psdk->devinfo.Sn_info.OEM_name, len, "%s", Oem_info->OEM_name);
		if (ret > 0 && ret < len)
		{
			if ('\n' == ay_psdk->devinfo.Sn_info.OEM_name[ret - 1]) 
				ay_psdk->devinfo.Sn_info.OEM_name[ret-1] = '\0';
		}

		len = sizeof(ay_psdk->devinfo.Sn_info.Model);
		ret = snprintf(ay_psdk->devinfo.Sn_info.Model, len, "%s", Oem_info->Model);
		if (ret > 0 && ret < len)
		{
			if('\n'==ay_psdk->devinfo.Sn_info.Model[ret-1]) 
				ay_psdk->devinfo.Sn_info.Model[ret-1] = '\0';
		}

		len = sizeof(ay_psdk->devinfo.Sn_info.Factory);
		ret = snprintf(ay_psdk->devinfo.Sn_info.Factory, len, "%s", Oem_info->Factory);
		if (ret > 0 && ret < len)
		{
			if ('\n' == ay_psdk->devinfo.Sn_info.Factory[ret - 1]) 
				ay_psdk->devinfo.Sn_info.Factory[ret - 1] = '\0';
		}
    }
}

void Ulu_SDK_Set_OEM_Info(Dev_SN_Info  *Oem_info)
{
    ay_set_oeminfo(Oem_info);
}

void Ulu_SDK_Set_Rtsp_Url(const char *url)
{
    if(url==NULL || strlen(url)==0)
	return ;

    if(sdk_init_instace(0))
    {
		strncpy(ay_psdk->devinfo.rtsp_url,url,127);
    }
}

void Ulu_SDK_Set_Video_size(int width, int height)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->devinfo.video_width = (uint16)width;
		ay_psdk->devinfo.video_height = (uint16)height;
    }
}

void Ulu_SDK_Enable_AutoSyncTime(void)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->devinfo.sync_time_flag = 0;
    }
}

void Ulu_SDK_Disable_AutoSyncTime(void)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->devinfo.sync_time_flag = 1;
    }
}

char *Ulu_SDK_Get_Device_ID(void)
{
    if (sdk_init_instace(0))
    {
		if (ay_psdk->devinfo.did.device_id_length == 0)
		{
			ayutil_read_file(ay_psdk->devinfo.rw_path, "SN_ulk", ay_psdk->devinfo.device_id_str, 20);
		}
		return (char*)ay_psdk->devinfo.device_id_str;
    }
    return NULL;
}

void Ulu_SDK_Param_Vedio_Upload(ULK_Video_Param_Ack  *video_param)
{
}

void Ulu_SDK_Code_Param_Vedio_Upload(ULK_Video_Encode_Param_Ack  *video_param)
{
}

int Ulu_SDK_Get_Version(void)
{
    unsigned char a, b, c;
    sscanf(RVERSION, "%hhu.%hhu.%hhu", &a ,&b, &c);
    return (a << 16) | (b << 8) | c;
}

static int ay_init_sdk(Dev_Attribut_Struct  *attr)
{
    if (sdk_init_instace(0) == NULL) {
        fprintf(stderr,"Ulk SDK malloc Instance Failed!\n");
        return -1;
    }

    ay_psdk->devinfo.dev_type = attr->dev_type;
    ay_psdk->devinfo.channelnum = attr->channel_num;
    if (ay_psdk->devinfo.channelnum > MAX_CHANNEL_NUM) {
        ay_psdk->devinfo.channelnum  = MAX_CHANNEL_NUM;
    }
    ay_psdk->devinfo.min_rate = attr->min_rate;
    ay_psdk->devinfo.max_rate = attr->max_rate;
    ay_psdk->devinfo.ptz_flag = attr->ptz_ctrl;
    ay_psdk->devinfo.mic_flag = attr->mic_flag;
    ay_psdk->devinfo.can_rec_voice = attr->can_rec_voice;
    ay_psdk->devinfo.hard_disk = attr->hard_disk;
    ay_psdk->devinfo.audio_type = attr->audio_type;
    ay_psdk->devinfo.audio_chnl = attr->audio_chnl;
    ay_psdk->devinfo.audio_smaple_rt= attr->audio_smaple_rt;
    ay_psdk->devinfo.audio_bit_width = attr->audio_bit_width;
    ay_psdk->devinfo.has_tfcard = attr->has_tfcard;
    ay_psdk->devinfo.video_type = attr->video_type;
    ay_psdk->devinfo.block_nums = attr->block_nums;
	ay_psdk->devinfo.max_stream_num_per_chn = attr->Reserved[0]; // add by che
	
    if (attr->use_type < 3) {
        ay_psdk->devinfo.use_type = attr->use_type;
    } else {
        ay_psdk->devinfo.use_type = 0;
    }
    if (attr->p_rw_path != NULL) {
        strncpy(ay_psdk->devinfo.rw_path, attr->p_rw_path, 127);
        int i = strlen(ay_psdk->devinfo.rw_path);
        if (ay_psdk->devinfo.rw_path[ i - 1] == '/') {
            ay_psdk->devinfo.rw_path[ i - 1] = 0;
        }
    } else {
        strcpy(ay_psdk->devinfo.rw_path, ".");
    }
#ifdef WIN32
    int err = network_init();
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return -1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    aydebug_vindicate_logfile();
    DEBUGF("LIB MAKE TIME----->>%s %s\n", __DATE__ , __TIME__);
#if AYDEVICE2_ENABLE
    TRACEF("[sdk api-use] new device2 api.\n");
#endif
#if USE_GETHOSTBYNAME
    DEBUGF("[sdk api-use] gethostbyname .\n");
#endif

    if (ay_init_device_object(&ay_psdk->devobj[0], attr->block_nums, 20, 30, 5) < 0) {
        ERRORF("Start|ay init device object fail!\n");
        ay_deinit_device_object(&ay_psdk->devobj[0]);
        pthread_mutex_destroy(&ay_psdk->frame_report_lock);
        free(ay_psdk);
        ay_psdk = NULL;
        return -1;
    }
    ayutil_save_version(ay_psdk->devinfo.rw_path,"verinfo");
    trace_init_net_stat_info();
    share_mem_init(); 
    init_stream_env();

    if (pool_init(3) < 0) {
        ERRORF("Start|pool init 3 fail!\n");
        goto init_fail;
    }
    pool_add_worker(&aydevice2_query_dnslit, 0);

    st_ay_thread_ctrl *pthds = &ay_psdk->threads;
    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, log_process_thread, NULL)) {
		ERRORF("{{DeviceCommEvent}}Start|log_process_thread thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"log_process_thread");
    pthds->num ++;

    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, Regist_thread_C3, NULL)) {
		ERRORF("{{DeviceCommEvent}}Start|Regist_thread_C3 thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"Regist_thread_C3");
    pthds->num ++;
    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, device_discovery_thread, NULL)) {
		ERRORF("{{DeviceCommEvent}}Start|device_discovery_thread thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"device_discovery_thread");
    pthds->num ++;
    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, Interact_CallBack_Thread, NULL)) {
		ERRORF("{{DeviceCommEvent}}Start|down cmd thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"Interact_CallBack_Thread");
    pthds->num ++;

    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, aystream_recv_thread, NULL)) {
        ERRORF("{{DeviceCommEvent}}Start|stream recv thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"aystream_recv_thread");
    pthds->num ++;
    if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, aystream_send_thread, NULL)) {
        ERRORF("{{DeviceCommEvent}}Start|stream send thread error(%s)!\n", strerror(errno));
		goto init_fail;
    }
    strcpy(pthds->tasks[pthds->num].name,"aystream_send_thread");
    pthds->num ++;

    if (attr->lan_disable) {
		TRACEF("user disable lan feature,val = %d!\n",attr->lan_disable);
    } else {
        if (ayutil_pthread_create(&(pthds->tasks[pthds->num].tid), NULL, lan_stream_thread, NULL)) {
            ERRORF("{{DeviceCommEvent}}Start|lan thread error(%s)!\n", strerror(errno));
            goto init_fail;
        }
        strcpy(pthds->tasks[pthds->num].name,"lan_stream_thread");
        pthds->num ++;
    }
    TRACEF("{{DeviceCommEvent}}Start|build %d-%d-%d!\n",YEAT,MONTH,DAYS);
    return 0;

init_fail:
    Ulu_SDK_DeInit();
    ERRORF("Failed|build %d-%02d-%02d\n", YEAT, MONTH, DAYS);
    return -1;
}

int Ulu_SDK_Init(Dev_Attribut_Struct  *attr)
{
	TRACEF("Ulu_SDK_Init build %d-%d-%d.............................\n", YEAT, MONTH, DAYS);
    return ay_init_sdk(attr);
}

int Ulu_SDK_Init_All(Dev_SN_Info *oem_info, Dev_Attribut_Struct *devattr, const char *logfile)
{
    if (oem_info == NULL || devattr == NULL) {
        return -1;
    }
    ay_enable_debug(logfile);
    ay_set_oeminfo(oem_info);
    return ay_init_sdk(devattr);
}

void Ulu_SDK_DeInit(void)
{
    int i = 0;

    if(ay_psdk!=NULL)
    {
		if(ay_psdk->exit_flag)
			return ;

		ay_psdk->exit_flag = 1;
		st_ay_thread_ctrl *pthds = &ay_psdk->threads;
		for(i=0;i<pthds->num;i++)
		{
			pthread_join(pthds->tasks[i].tid,NULL);
			DEBUGF("thread[%d] %s exit ok!\n",i,pthds->tasks[i].name);
		}
		ay_deinit_device_object(&ay_psdk->devobj[0]);
		pool_destroy();
		exit_stream_env();
		share_mem_uint();

		if(ay_psdk->debug.log_cache.cap > 0)
		{
			free(ay_psdk->debug.log_cache.pbuf);
			ay_psdk->debug.log_cache.pbuf = 0;
			ay_psdk->debug.log_cache.cap = 0;
			ay_psdk->debug.log_cache.size = 0;
			ay_psdk->debug.log_cache.woffset = 0;
			ay_psdk->debug.log_cache.roffset = -1;
//			ay_psdk->debug.log_cache.mutex = 0;
			pthread_mutex_destroy(&ay_psdk->debug.log_cache.mutex);


		}
		if(ay_psdk->debug.log_fd!=NULL)
		{
			fclose(ay_psdk->debug.log_fd);
			ay_psdk->debug.log_fd = NULL;
		}

		pthread_mutex_destroy(&ay_psdk->frame_report_lock);
		free(ay_psdk);
		ay_psdk = NULL;
#ifdef WIN32
		WSACleanup();
#endif
    }
}

void Ulu_SDK_Set_Interact_CallBack(Interact_CallBack callback_fun)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->cbfuncs.pOemCbfc = ay_parse_oem_cmd;
		ay_psdk->cbfuncs.pCmdCbfc = callback_fun;
    }
}

void Ulu_SDK_Set_AudioAAC_CallBack(Audio_AAC_CallBack callback_fun)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->cbfuncs.pAudioCbfc = callback_fun;
    }
}

void Ulu_SDK_Set_Talk_CallBack(Audio_Talk_CallBack callback_fun)
{
    if(sdk_init_instace(0))
    {
		ay_psdk->cbfuncs.pAudioCbfc = callback_fun;
    }
}

int Ulu_SDK_Set_WIFI_CallBack(WIFI_CallBack callback_fun)
{
    if(sdk_init_instace(0) && callback_fun)
    {
		printf("[ulk ulink]set WIFI_CallBack\n");		
		ay_psdk->cbfuncs.pWifiCbfc = callback_fun;
		ay_psdk->wifi.is_request_encrypt = 1;
		return 0;
    }
    return -1;
}

int  Ulu_SDK_Set_WIFI_CallBack_Ext(WIFI_CallBack callback_fun, char request_encrypt, char *wireless_lan_name)
{
    if(sdk_init_instace(0) && callback_fun && wireless_lan_name)
    {
		printf("[ulk ulink]set wificallback ext,name = %s\n",wireless_lan_name);		
		st_ay_wifi_ctrl *pwifi = &ay_psdk->wifi;
		ay_psdk->cbfuncs.pWifiCbfc = callback_fun;

		pwifi->is_request_encrypt = !!request_encrypt; // 0 or 1

		memset(pwifi->wireless_name, 0, sizeof(pwifi->wireless_name));
		strncpy(pwifi->wireless_name, wireless_lan_name,sizeof(pwifi->wireless_name)-1);
		return 0;		
    }
    return -1;
}

int Ulu_SDK_Send_Userdata(uint8 *pdata, uint16 len)
{
    char   buf[1024+20];
    int    buf_len;
    MsgStatusReport   msg;
    if ( len > 1024 )
    {
		return -1;
    }

    msg.flag = 0x00;
    msg.mask = 0x20;
    msg.oem_data_len = len;
    msg.oem_data = pdata;

    buf_len = Pack_MsgStatusReport(buf, &msg);
    if ( buf_len > 0 )
    {
		Msg_Cmd_Add_queue(buf, buf_len);
    }
    return 0;
}

int Ulu_SDK_History_Srch_Rslt_None(uint8 chn)
{
    return History_Srch_Rslt_Ack(0,chn);
}

int Ulu_SDK_Get_Device_Online(void)
{
    return (aystream_get_status()==1)?true:false;
}

int  Ulu_SDK_Screen_Shot_Upload(Screen_Shot_Struct *ppic)
{
    static struct timespec period = {0,0};
    int permission;

    if(NULL == ppic)
    {
		return -1;
    }
    else if ( ppic->channel_index< 1 || ppic->channel_index > MAX_CHANNEL_NUM )
    {
		return -5;
    }
    else if(aystream_get_status() < 0)
    {
		return -4;
    }

    permission = Get_Alarm_Permission(&sdk_init_instace(0)->alarm_ctrl,period);
    if ( permission == 0)
    {
		if(sdk_get_handle(0)->inst_flag&SDK_FLAG_UPLOAD_SNAP)
		{
			return AY_ERROR_RUNING;
		}
		Screen_Shot_Struct *psnap = (Screen_Shot_Struct*)malloc(sizeof(Screen_Shot_Struct));
		if(NULL == psnap)
		{
			return AY_ERROR_MEMORY;
		}
		memcpy(psnap,ppic,sizeof(Screen_Shot_Struct));
		psnap->pic_data = (uint8*)malloc(ppic->pic_length);
		if(NULL == psnap->pic_data)
		{
			free(psnap);
			return AY_ERROR_MEMORY;
		}
		memcpy(psnap->pic_data,ppic->pic_data,ppic->pic_length);

		clock_gettime(CLOCK_MONOTONIC,&period);
		sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_SNAP;
		pool_add_worker(&upload_screen_snap,(int)psnap);
		return 0;
    }
    else
    {
	return permission;
    }	   
}

int Ulu_SDK_Alarm_Upload_V4(int channel, ALARM_TYPE alarm_type, uint32 alarm_time, ULK_ALARM_STATE alarm_state)
{
	int permission, len, i = 0, j = 0;
	char    buf[512 + 256];
	MsgStatusReport  status_msg;

	static struct timespec period = { 0, 0 };

	if (sdk_init_instace(0) == NULL)
	{
		return -1;
	}

	if (channel < 1 || channel > MAX_CHANNEL_NUM)
	{
		return -5;
	}
	pthread_mutex_lock(&sdk_get_handle(0)->stream_flag_lock);
	sdk_get_handle(0)->alarm_state = 0;
	if (alarm_state == ALARM_ON)
	{
		sdk_get_handle(0)->alarm_state = 1;
	}
	else if (alarm_state == ALARM_OFF)
	{
		pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);
		return 0;
	}
	if (alarm_type & HUMAN_DETECT)
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		sdk_get_handle(0)->human_detect_start = ts.tv_sec;
		sdk_get_handle(0)->alarm_time = alarm_time == 0 ? time(NULL) : alarm_time;
		if (!(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_HUMAN_STREAM))
		{
			sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_HUMAN_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_HUMAN);
		}

	}

	if (ay_psdk->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE)
	{// set
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		sdk_get_handle(0)->mdcloud_start = ts.tv_sec;
		if (!(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM))
		{
			sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_MD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
			if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
				add_channel_cmd(AUDIO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	else
	{
		if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM)
		{	// clear
			sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);

	permission = Get_Alarm_Permission(&sdk_init_instace(0)->alarm_ctrl, period);
	if (permission == 0)
	{
		ay_psdk->devinfo.chnl_status[channel - 1].channel_index = channel;
		STATUS_SET_EXT(ay_psdk->devinfo.chnl_status[channel - 1].channel_status, (enum DeviceStatus)alarm_type);

		DEBUGF("alarm_Report..\n");
		clock_gettime(CLOCK_MONOTONIC, &period);
		status_msg.mask = 0;
		for (i = 0; i < MAX_CHANNEL_NUM; i++)
		{
			if (0 != ay_psdk->devinfo.chnl_status[i].channel_status)
			{
				status_msg.channels_status[j].channel_status = ay_psdk->devinfo.chnl_status[i].channel_status;
				status_msg.channels_status[j].channel_index = i + 1;
				j++;
				ay_psdk->devinfo.chnl_status[i].channel_status = 0;
			}
		}
		status_msg.channel_status_num = j;
		if (j > 0)
		{
			status_msg.mask |= 0x40;
			DEBUGF("pack alarm\n");
		}
		len = Pack_MsgStatusReport(buf, &status_msg);
		Msg_Cmd_Add_queue(buf, len);
		return 0;
	}
	else
	{
		return permission;
	}
}

int Ulu_SDK_Alarm_Upload_V3(int channel, ALARM_TYPE  alarm_type, uint32 alarm_time)
{
	int permission, len, i = 0, j = 0;
	char    buf[512 + 256];
	MsgStatusReport  status_msg;

	static struct timespec period = { 0, 0 };

	if (sdk_init_instace(0) == NULL)
	{
		return -1;
	}

	if (channel < 1 || channel > MAX_CHANNEL_NUM)
	{
		return -5;
	}
	pthread_mutex_lock(&sdk_get_handle(0)->stream_flag_lock);
	if (alarm_type & HUMAN_DETECT)
	{
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		sdk_get_handle(0)->human_detect_start = ts.tv_sec;
		sdk_get_handle(0)->alarm_time = alarm_time == 0 ? time(NULL) - (8 * 3600) : alarm_time;  //time zone
		if (!(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_HUMAN_STREAM))
		{
			sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_HUMAN_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_HUMAN);
		}

	}

	if (ay_psdk->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE)
	{// set
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		sdk_get_handle(0)->mdcloud_start = ts.tv_sec;
		if (!(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM))
		{
			sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_MD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
			if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
				add_channel_cmd(AUDIO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	else
	{
		if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM)
		{	// clear
			sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);

	permission = Get_Alarm_Permission(&sdk_init_instace(0)->alarm_ctrl, period);
	if (permission == 0)
	{
		ay_psdk->devinfo.chnl_status[channel - 1].channel_index = channel;
		STATUS_SET_EXT(ay_psdk->devinfo.chnl_status[channel - 1].channel_status, (enum DeviceStatus)alarm_type);

		DEBUGF("alarm_Report..\n");
		clock_gettime(CLOCK_MONOTONIC, &period);
		status_msg.mask = 0;
		for (i = 0; i < MAX_CHANNEL_NUM; i++)
		{
			if (0 != ay_psdk->devinfo.chnl_status[i].channel_status)
			{
				status_msg.channels_status[j].channel_status = ay_psdk->devinfo.chnl_status[i].channel_status;
				status_msg.channels_status[j].channel_index = i + 1;
				j++;
				ay_psdk->devinfo.chnl_status[i].channel_status = 0;
			}
		}
		status_msg.channel_status_num = j;
		if (j > 0)
		{
			status_msg.mask |= 0x40;
			DEBUGF("pack alarm\n");
		}
		len = Pack_MsgStatusReport(buf, &status_msg);
		Msg_Cmd_Add_queue(buf, len);
		return 0;
	}
	else
	{
		return permission;
	}
}

int  Ulu_SDK_Alarm_Upload_Ex(int channel, ALARM_TYPE  alarm_type)
{
	return Ulu_SDK_Alarm_Upload_V3(channel, alarm_type, 0);
}

void Ulu_SDK_Alarm_Upload(ALARM_TYPE  alarm_type)
{
    Ulu_SDK_Alarm_Upload_V3(1, alarm_type, 0);
}

int  Ulu_SDK_Alarm_Stream_Start(int channel)
{
    int permission, len, i = 0,j = 0;
    char    buf[512+256];
    //MsgStatusReport  status_msg;

    static struct timespec period = {0,0};

    if(sdk_init_instace(0) == NULL)
    {
    	printf("sdk_init_instace = NULL\n");
		return -1;
    }

    if ( channel < 1 || channel > MAX_CHANNEL_NUM)
    {
		return -5;
    }
	
	pthread_mutex_lock(&sdk_get_handle(0)->stream_flag_lock);
	
	if (ay_psdk->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE)
	{// set
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		//sdk_get_handle(0)->mdcloud_start = ts.tv_sec;
		sdk_get_handle(0)->alarm_state = 1;
		DEBUGF("Ulu_SDK_Alarm_Stream_Start 1\n");
		if (!(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_HD_STREAM))
		{
			DEBUGF("Ulu_SDK_Alarm_Stream_Start 2\n");
			sdk_get_handle(0)->inst_flag |= SDK_FLAG_UPLOAD_HD_STREAM;
			add_channel_cmd(VIDEO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
			if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
				add_channel_cmd(AUDIO_CTRL, 1, 1, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	else
	{
		DEBUGF("Ulu_SDK_Alarm_Stream_Start 3\n");
		if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_HD_STREAM)
		{	// clear
			DEBUGF("Ulu_SDK_Alarm_Stream_Start 4\n");
			//sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_HD_STREAM;
			sdk_get_handle(0)->alarm_state = 0;
			//add_channel_cmd(VIDEO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
			//if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
			//	add_channel_cmd(AUDIO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		}
	}
	pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);
	
    return 0;
}

int  Ulu_SDK_Alarm_Stream_End(int channel)
{
    int permission, len, i = 0,j = 0;
    char    buf[512+256];
    MsgStatusReport  status_msg;

    static struct timespec period = {0,0};

    if(sdk_init_instace(0) == NULL)
    {
    	printf("sdk_init_instace = NULL\n");
		return -1;
    }

    if ( channel < 1 || channel > MAX_CHANNEL_NUM)
    {
		return -5;
    }

	pthread_mutex_lock(&sdk_get_handle(0)->stream_flag_lock);

	DEBUGF("Ulu_SDK_Alarm_Stream_End 1\n");
	if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_HD_STREAM)
	{// clear
		DEBUGF("Ulu_SDK_Alarm_Stream_End 2\n");
		//sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_HD_STREAM;
		sdk_get_handle(0)->alarm_state = 0;
		//add_channel_cmd(VIDEO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
		//if (ay_psdk->devinfo.vip_flag & C3VP_UPLOAD_AUDIO)
		//	add_channel_cmd(AUDIO_CTRL, 1, 0, ay_psdk->devinfo.min_rate, CMD_FROM_MDCLOUD);
	}

	pthread_mutex_unlock(&sdk_get_handle(0)->stream_flag_lock);

	return 0;
}


uint64 Ulu_SDK_GetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec/(1000*1000));
}

void  Ulu_SDK_Get_Upload_Speed(uint32 *send_speed, uint32 *send_ok_speed)
{
    struct net_statistic_info netinfo;
    trace_get_net_stat_info(&netinfo);

    *send_speed = (uint32)netinfo.want_send_speed[1];
    *send_ok_speed = (uint32)netinfo.real_send_speed[1];
}

int  Ulu_SDK_Get_Connect_Status(void)
{
    return  aystream_get_status();
}

void Ulu_SDK_Reset_Stream_Links(void)
{
    TRACEF("{{StreamConnEvent}}DisconnectActive|reset stream links\n");
    aystream_reset_conns();
}

int  Ulu_SDK_History_List_Send(HistoryListQuery_Struct *pEvent)
{
    if(sdk_get_handle(0) == NULL)
    {
		return -1;
    }

    DEBUGF("pEvent=%d\n",pEvent->list_len);
    return History_List_Upload(pEvent,0);
}

void Ulu_SDK_Send_Zoom_Value(uint8 chn, uint32 Zoom_value)
{
    char   buf[1024+20];
    int    buf_len;
    MsgStatusReport   msg;

    msg.flag = 0x00;
    msg.mask = 0x100;
    msg.channel_index_zoom = chn;
    msg.zoom_value = Zoom_value*10;

    buf_len = Pack_MsgStatusReport(buf, &msg);
    if ( buf_len > 0 )
    {
		Msg_Cmd_Add_queue(buf, buf_len);
    }
    return ;
}

/* args: 0 - cancel, 1 - set */
void Ulu_SDK_Device_Private_Set(uint8 args)
{
    if(sdk_get_handle(0) == NULL)
    {
		return ;
    }

    uint32 mask = ay_psdk->devinfo.dev_status_mask & ON_PRIVATE_MODE;
    if(args && !mask)
    {// set private(close -> open)
		ay_psdk->devinfo.dev_status_mask |= ON_PRIVATE_MODE;
		ay_psdk->devinfo.devstatus_update_flag = 1;
    }
    else if(!args && mask)
    {// cancel private(open -> close)
		ay_psdk->devinfo.dev_status_mask &= ~(ON_PRIVATE_MODE);
		ay_psdk->devinfo.devstatus_update_flag = 1;
    }
}

int Ulu_SDK_Device_Record_Report(AYE_TYPE_SD_STATUS status,AYE_TYPE_RECORD_MODE mode)
{
    char   buf[1024+20];
    int    buf_len;
    MsgStatusReport   msg;

    if(sdk_get_handle(0) == NULL)
    {
		return -1;
    }

    ay_psdk->devinfo.sdcard_status = status;
    ay_psdk->devinfo.record_mode = mode;
    if(status == AYE_VAL_SD_NEXSIT)
	ay_psdk->devinfo.has_tfcard = 0;
    else 
	ay_psdk->devinfo.has_tfcard = 1;

    if( aystream_get_status() > 0 )
    {
		msg.flag = 0x00;
		msg.mask = 0x200;
		msg.sdcard_status = status;
		msg.record_mode = mode;

		buf_len = Pack_MsgStatusReport(buf, &msg);
		if ( buf_len > 0 )
		{
			Msg_Cmd_Add_queue(buf, buf_len);
		}
		return 0;
    }
    return -1;
}

int Ulu_SDK_NVR_Init(int  blocknums,int  ts_mem_size)
{
    DEBUGF("Ulu_SDK_NVR_Init unused,blocknums:%d,ts_mem_size:%d\n",blocknums,ts_mem_size);
    return  0;
}

int  Ulu_SDK_History_Frame_Send(Stream_History_Frm_Struct *pfrm_data, ULK_HISTORY_VIDEO_ENUM  is_end)
{
    st_ay_history_ctrl *phist = &sdk_get_handle(0)->history;
    uint8 chn = pfrm_data->channelnum;

    if((phist->his_info[chn-1].his_flag==1)&&(pfrm_data!=NULL))
    {
        Stream_Event_Struct pEvent;

		if(phist->his_info[chn-1].his_enable==1)
		{
			DEBUGF("hist chn[%d] enable=%d\n",chn,phist->his_info[chn-1].his_enable);
			History_Srch_Rslt_Ack(1, pfrm_data->channelnum);
			phist->his_info[chn-1].his_enable = 0;
		}

		if(IS_LIVE_FRAME(pfrm_data->frm_type))
		{
			if(pfrm_data->frm_type==CH_I_FRM)
			{
				pfrm_data->frm_type=CH_HIS_I_FRM;
			}
			else if(pfrm_data->frm_type==CH_P_FRM)
			{
				pfrm_data->frm_type=CH_HIS_P_FRM;
			}
			else
			{
				pfrm_data->frm_type=CH_HIS_AUDIO_FRM;
			}
		}
        pEvent.channelnum=pfrm_data->channelnum;
        pEvent.bit_rate=pfrm_data->bit_rate;
        pEvent.frm_ts=pfrm_data->frm_ts;
        pEvent.pdata=(char*)pfrm_data->pdata;
        pEvent.frm_size=pfrm_data->len;
        pEvent.frm_type=pfrm_data->frm_type;
	pEvent.media_video_type = pfrm_data->media_video_type;
	pEvent.media_audio_type = pfrm_data->media_audio_type;

        return Ulu_SDK_Stream_Event_Report(&pEvent);
    }
    else
    {
		DEBUGF(">>>> history flag = %d,(ts stream deprecated)!\n",phist->his_info[chn-1].his_flag);
		return -1;
    }
}

const int ay_stream_brate[4] = {UPLOAD_RATE_1,UPLOAD_RATE_2,UPLOAD_RATE_3,UPLOAD_RATE_4};
int  Ulu_SDK_Stream_Event_Report(Stream_Event_Struct *pEvent)
{
    unsigned int rate_index = 0;
    int rlst = 0;

    if ( pEvent->channelnum < 1 || pEvent->channelnum > MAX_CHANNEL_NUM )
    {
		return -3;
    }
    rate_index = ayutil_query_rate_index(pEvent->bit_rate);
    if(rate_index == -1 )
    {
		return -4;
    }
    if( !(IS_LIVE_FRAME(pEvent->frm_type)||IS_HIST_FRAME(pEvent->frm_type)))
    {
		return -6;
    }
    pEvent->bit_rate = ay_stream_brate[rate_index]; /* correct bit rate to sdk need */
    rlst = aystream_add_frame(&ay_psdk->devobj[1],pEvent); // LAN stream

    if(aystream_get_status() < 0)
    {
		return -1;
    }
    rlst = aystream_add_frame(&ay_psdk->devobj[0],pEvent); // WAN stream
    return rlst;
}

/*
 *  获取设备报警控制参数
 *  @param[in] type 0 - motion alarm, 1 - audio alarm
 *  @param[in,out] pctrl return the alarm ctrl parameters if found the alarm type
 *  @ret return 0 when found, or return -1.
 */
int Ulu_SDK_Get_Alarm_Ctrl(uint8 type,ULK_Alarm_Struct *pctrl)
{
    if(ay_psdk && pctrl)
    {
		int i = 0;
		for(i=0;i<sizeof(ay_psdk->alarm_ctrl)/sizeof(ULK_Alarm_Struct);i++)
		{
			if(type == ay_psdk->alarm_ctrl.alarm_type)
			{
				memcpy(pctrl,&ay_psdk->alarm_ctrl,sizeof(ULK_Alarm_Struct));
				return 0;
			}
		}
    }
    return -1;
}

/*  上传侦测报警记录到服务器
 *  @ret -5:通道范围不对  -4:没有连接  -3:周期太短 -2:时间范围不允许 -1:服务器禁止报警.0 正常
 */
int Ulu_SDK_Upload_Alarm_Record(int channel, ALARM_TYPE  alarm_type, ULK_Alarm_Clock_struct alarm_rec)
{
    return 0;
}

int Ulu_SDK_Reset_Device(void)
{
    if(sdk_get_handle(0) == NULL)
    {
		return -1;
    }
    if(strlen(ay_psdk->devinfo.device_id_str)==0 || ay_psdk->devinfo.entry_token_b64.token_bin_length==0)
    {
		ERRORF("device_id or entry token empty!\n");
		return -1;
    }
#if 0
    char entry_token_base64[512]={0};
    int len = sizeof(entry_token_base64),ret = 0;
    ret = ZBase64Encode((const char*)ay_psdk->devinfo.entry_token_b64.token_bin,ay_psdk->devinfo.entry_token_b64.token_bin_length,entry_token_base64,&len);
    if(ret < 0)
    {
		//DEBUGF("token_bin:%s,len:%d\n",ay_psdk->devinfo.entry_token_b64.token_bin,ay_psdk->devinfo.entry_token_b64.token_bin_length);
		return -1;
    }
    DEBUGF("entry token:%s, len:%d\n",entry_token_base64,len);
#endif
    return ay_request_reset_device(ay_psdk->devinfo.device_id_str,ay_psdk->devinfo.entry_token_base64);
}

int Ulu_SDK_Set_20SN(const char *sn20)
{
    if (ay_psdk && sn20 && strlen(sn20) >= 20) {
        ayutil_save_file(ay_psdk->devinfo.rw_path, "SN_ulk", sn20, strlen(sn20));
        return 0;
    }
    return -1;
}
