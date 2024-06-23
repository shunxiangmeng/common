#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "ayutil.h"
#include "ayqueue.h"
#include "ay_sdk.h"
#include "channel.h"

static int open_chn_rate(Chnl_ctrl_table_struct *ptable,int channel_index, int  upload_rate, int av)
{
    int index = ayutil_query_rate_index(upload_rate);
    int bit = av&0x03; // video and audio
    printf("open_chn_rate bit = %d channel_index = %d index = %d\n",bit,channel_index,index);

    // FIX me: we cannot distinguish different client's different video or audio request.
    if(!(bit & ptable[channel_index - 1].bit_r[index]))
    {/* only when channel status change */
        ptable[channel_index - 1].bit_r[index] |= bit;
        if(0x01 == av) ptable[channel_index - 1].frm_id[index].first_I_frame_required = 1; // only video
    }
    bit = av&0xff00; // cmd_from 
    if(!(bit & ptable[channel_index - 1].bit_r[index]))
    {
        ptable[channel_index - 1].bit_r[index] |= bit;
    }
    printf("open_chn_rate bit = %d\n",bit);
    return 1;
}
static int close_chn_rate(Chnl_ctrl_table_struct *ptable,int channel_index, int  upload_rate,int av)
{
    int index = ayutil_query_rate_index(upload_rate);
    int bit = av&0xff00;
	printf("close_chn_rate bit = %d channel_index = %d index = %d\n",bit,channel_index,index);

    // FIX me: we cannot distinguish different client's different video or audio request.
    if(bit & ptable[channel_index - 1].bit_r[index])
    {
	ptable[channel_index - 1].bit_r[index] &= ~bit;
	printf("bit_r = %d\n",ptable[channel_index - 1].bit_r[index]);
    }
    if(ptable[channel_index - 1].bit_r[index] & 0xff00)
    {// has another request-client need the channel stream
	return 0;
    }

    bit = av&0x03;
    if(bit & ptable[channel_index - 1].bit_r[index])
    {/* only when channel status change */
	ptable[channel_index - 1].bit_r[index] &= ~bit;
	if(0x01==av)
	{
	    ptable[channel_index - 1].frm_id[index].last_time_stamp = 0;
	    ptable[channel_index - 1].frm_id[index].time_ms_ref = 0;
	    ptable[channel_index - 1].frm_id[index].time_second_ms_base= 0;
	    ptable[channel_index - 1].frm_id[index].first_I_frame_required = 0;
	}
    }
	if (0x01 == av)
	{
		ptable[channel_index - 1].frm_id[index].now_send_I_ts = 0;
	}
    return 1;
}

static int  Set_Channel_ctrl_Table(Chnl_ctrl_table_struct *ptable,int channel_index, int enable, int  upload_rate,int av)
{
    int ch,ret = 0;
    int channelNums = sdk_get_handle(0)->devinfo.channelnum;

    if (enable == 1)                /* 传所有开关使能 */
    {
        if (channel_index == 0 )    /* 上传所有通道数据 */
        {
            if(upload_rate == 0)
            { /* 上传所有波特率 */
                DEBUGF("--->*open all chnl all rate-%d,%d\n", channel_index, upload_rate);
                for (ch = 1; ch <= channelNums; ch++)
                {
                    ret |= open_chn_rate(ptable,ch, UPLOAD_RATE_1,av);
                    ret |= open_chn_rate(ptable,ch, UPLOAD_RATE_2,av);
                    ret |= open_chn_rate(ptable,ch, UPLOAD_RATE_3,av);
                    ret |= open_chn_rate(ptable,ch, UPLOAD_RATE_4,av);
                }
            }
            else
            { /* 打开所有通道指定码率数据 */
                for (ch = 1; ch <= channelNums; ch++)
                {
                    ret |= open_chn_rate(ptable,ch, upload_rate,av);
                }
            }
        }
        else
        { /* 打开指定通道数据 */
            ret = open_chn_rate(ptable,channel_index, upload_rate,av);
        }
    }
    else if (enable == 0)           /* 关闭上传所有开关使能 */
    {
        if (channel_index == 0 )    /* 关闭所有通道数据 */
        {
            if(upload_rate == 0)
            { /* 关闭所有波特率 */
                DEBUGF("--->*close all chnl all rate-%d,%d\n",channel_index, upload_rate);
                for (ch = 1; ch <= channelNums; ch++)
                {
                    ret |= close_chn_rate(ptable,ch, UPLOAD_RATE_1,av);
                    ret |= close_chn_rate(ptable,ch, UPLOAD_RATE_2,av);
                    ret |= close_chn_rate(ptable,ch, UPLOAD_RATE_3,av);
                    ret |= close_chn_rate(ptable,ch, UPLOAD_RATE_4,av);
                }
            }
            else
            { /* 关闭指定码率数据 */
                for (ch = 1; ch <= channelNums; ch++)
                {
                    ret |= close_chn_rate(ptable,ch, upload_rate,av);
                }
            }
        }
        else    /* 关闭指定通道数据和波特率 */
        {
            ret = close_chn_rate(ptable,channel_index, upload_rate,av);
        }
    }
    return ret;
}

void Clean_All_Chnl_Second_Base(Chnl_ctrl_table_struct *ptable)
{
    int  i;
    for ( i = 0 ; i < MAX_CHANNEL_NUM; i++ )
    {
	ptable[i].frm_id[0].time_second_ms_base = 0;
	ptable[i].frm_id[1].time_second_ms_base = 0;
	ptable[i].frm_id[2].time_second_ms_base = 0;
	ptable[i].frm_id[3].time_second_ms_base = 0;

	ptable[i].frm_id[0].time_second_ms_base_audio= 0;
	ptable[i].frm_id[1].time_second_ms_base_audio = 0;
	ptable[i].frm_id[2].time_second_ms_base_audio = 0;
	ptable[i].frm_id[3].time_second_ms_base_audio = 0;
    }
}

int Get_All_chnl_status(Chnl_ctrl_table_struct *ptable)
{
    int   i;

    for ( i = 0 ; i < sdk_get_handle(0)->devinfo.channelnum; i++ )
    {
	if(ptable[i].bit_r[0] != 0 ||  ptable[i].bit_r[1] != 0
		||  ptable[i].bit_r[2] != 0 ||  ptable[i].bit_r[3] != 0)
	{
	    return 1;
	}
    }
    return 0;
}

static int query_chnl_status(Chnl_ctrl_table_struct *ptable,uint32 cid,uint32 chn,uint16 rate)
{
    int i = 0,index = 0;

    if(chn==0 && rate==0)
    {
	return Get_All_chnl_status(ptable);
    }
    else if(rate == 0)
    {
	for(i=0;i<4;i++)
	{
	    if(ptable[chn].bit_r[i])
		return 1;
	}
    }
    else if(chn == 0)
    {
	index = ayutil_query_rate_index(rate);
	for(i=0;i<MAX_CHANNEL_NUM;i++)
	{
	    if(ptable[i].bit_r[index])
		return 1;
	}
    }
    else
    {
	index = ayutil_query_rate_index(rate);
	if(ptable[chn].bit_r[index])
	    return 1;
    }
    return 0;
}


int add_channel_cmd(uint32 cid, uint32 chnlnum, uint32 enable, uint32  rate,uint32 cmd_from)
{
    int ret = 0;
    char *fromstr = NULL;
    CMD_PARAM_STRUCT   cmd;
    Chnl_ctrl_table_struct *ptable = sdk_get_handle(0)->devobj[0].streamFilter;

    switch (cmd_from)
    {
        case CMD_FROM_CLIENT: fromstr = "client"; break;
        case CMD_FROM_SERVER: fromstr = "server"; break;
        case CMD_FROM_MDCLOUD: fromstr = "mdclout"; break;
        case CMD_FROM_HUMAN: fromstr = "mdhuman"; break;
        default: fromstr = "unknown"; break;
    }

    if (cmd_from == CMD_FROM_CLIENT)
    {
        ptable = sdk_get_handle(0)->devobj[1].streamFilter;
    }

    cmd.channel =  chnlnum;
    cmd.cmd_id	= cid;

    struct in_addr ip;
    ip.s_addr = sdk_get_handle(0)->devinfo.stream_serv_ip;
    switch(cid)
    {
	case VIDEO_CTRL:
	    ret = Set_Channel_ctrl_Table(ptable,chnlnum, enable, rate,0x01|cmd_from);
	    cmd.cmd_args[0] = (uint8)enable;
	    cmd.cmd_args[1] = (uint8)(rate);
	    cmd.cmd_args[2] = (uint8)(rate>> 8);
	    DEBUGF("[video req]ch=%d,en=%d,rate=%d,cmd_from = %#x\n", chnlnum, enable, rate,cmd_from);
	    TRACEF("{{MediaEvent}}%d|%d|%s|Live|Video|%s|from:%s\n",chnlnum,rate,inet_ntoa(ip),enable?"Start":"Stop",fromstr);
	    break;
	case AUDIO_CTRL:
	    ret = Set_Channel_ctrl_Table(ptable,chnlnum, enable, rate,0x02|cmd_from);
	    cmd.cmd_args[0] = (uint8)enable;
	    cmd.cmd_args[1] = (uint8)(rate);
	    cmd.cmd_args[2] = (uint8)(rate>> 8);
	    DEBUGF("[audio req]ch=%d,en=%d,rate=%d,cmd_from = %#x\n", chnlnum, enable, rate,cmd_from);
	    TRACEF("{{MediaEvent}}%d|%d|%s|Live|Audio|%s|from:%s\n",chnlnum,rate,inet_ntoa(ip),enable?"Start":"Stop",fromstr);
	    break;
	case HISTORY_STOP:
        if (chnlnum > 0)
        {
            ret = 1;
            ptable[chnlnum - 1].bit_his_r = 0;
            TRACEF("{{MediaEvent}}%d|%d|%s|Playback|All|%s|from:%s\n",chnlnum,rate,inet_ntoa(ip),enable?"Start":"Stop",fromstr);
        }
        DEBUGF("[clr hist]ch=%d,en=%d,rate=%d\n", chnlnum, enable, rate);
        break;
    default:
        return -1;
    }

	DEBUGF("ret = %d enable = %d,then query_chnl_status\n",ret,enable);

    if (ret && !enable) // close we need think of another devobj
    {
        if (cmd_from == CMD_FROM_CLIENT &&
            query_chnl_status(sdk_get_handle(0)->devobj[0].streamFilter,cid,chnlnum,rate))
        {// WAN still open
            DEBUGF("WAN still open..chn[%d],rate[%d]\n",chnlnum,rate);
            ret = 0;
        }
        else if (query_chnl_status(sdk_get_handle(0)->devobj[1].streamFilter,cid,chnlnum,rate))
        {// LAN still open
            DEBUGF("LAN still open..chn[%d],rate[%d]\n",chnlnum,rate);
            ret = 0;
        }
    }

    DEBUGF("ret = %d then Msg_Cmd_Add_down_queue\n",ret);

    if (ret) {
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }
    return 0;
}

int  PTZ_Channel_ctrl(uint8 ch,uint32  ptzctrl, uint32 steps)
{
    CMD_PARAM_STRUCT   cmd;
    cmd.channel =  ch;
    cmd.cmd_id	= PTZ_CTRL;

    switch(ptzctrl)
    {
    case enm_device_left:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_LEFT; // 1
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_right:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_RIGHT; // 2
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_up:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_UP; // 3
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_down:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_DOWN; // 4
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_zoom_in:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_ZOOM_IN; // 5
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_zoom_out:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_ZOOM_OUT; // 6
	    cmd.cmd_args_len = 2;
	    break;
	case enm_device_stop:
	    cmd.cmd_args[0] = 0;
	    cmd.cmd_args[1] = 0;
	    cmd.cmd_args_len = 2;
	    break;

	case enm_server_reset_device:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_SYS_RESET; // 10
	    cmd.cmd_args_len = 2;
	    break;
	case enm_server_reboot_device:
	    cmd.cmd_args[0] = 1;
	    cmd.cmd_args[1] = PTZ_CTRL_SYS_REBOOT; // 11
	    cmd.cmd_args_len = 2;
	    break;
	default:
	    cmd.cmd_args_len = 0;
	    break;
    }

    cmd.cmd_args[cmd.cmd_args_len++] = steps&0xff;
    cmd.cmd_args[cmd.cmd_args_len++] = (steps>>8)&0xff;
    cmd.cmd_args[cmd.cmd_args_len++] = (steps>>16)&0xff;
    cmd.cmd_args[cmd.cmd_args_len++] = (steps>>24)&0xff;

    Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    return 0;
}

void  deal_talk_thread(int arg)
{
    Audio_Info_struct   *tmp_info;
    MsgD2SUserDataRequest   req_msg;
    char req_msg_buf[384], temp_info_buf[384];
    int i, t, len;

    Audio_Talk_CallBack  ptalk_cb = sdk_get_handle(0)->cbfuncs.pAudioCbfc;
    Audio_Talk_Buf_Struct *ptalk_buf = &sdk_get_handle(0)->talk.talk_buf;
    Audio_Talk_Ctrl_Struct *ptalk_ctrl = &sdk_get_handle(0)->talk.talk_ctrl;

    while(1)
    {
	len = Audio_msg_Get_queue(temp_info_buf);
	if(len == -1)
	{
	    break;
	}

	tmp_info = (Audio_Info_struct *)temp_info_buf;
	ptalk_buf->seq = tmp_info->seq;

	if(tmp_info->from == CMD_FROM_SERVER)
	{
	    /*
	     * 向服务器请求客户端发送过来的对讲音频数据
	     * 请求一包，服务器发送一包给设备
	     */
	    req_msg.mask = 0x01;
	    req_msg.user_name = tmp_info->user_name;
	    req_msg.seq = tmp_info->seq;
	    t = tmp_info->audio_total_len / FRAME_AUDIO_FRM_SIZE;
	    for (  i = 0; i < t ;i++ )
	    {
		req_msg.audio_offset = i * FRAME_AUDIO_FRM_SIZE;
		req_msg.audio_len = FRAME_AUDIO_FRM_SIZE;
		len = Pack_MsgD2SUserDataRequest(req_msg_buf, &req_msg);
		Msg_Cmd_Add_queue(req_msg_buf, len);
	    }
	    if ((tmp_info->audio_total_len % FRAME_AUDIO_FRM_SIZE) > 0)
	    {
		req_msg.audio_offset = i * FRAME_AUDIO_FRM_SIZE;
		req_msg.audio_len = tmp_info->audio_total_len % FRAME_AUDIO_FRM_SIZE;
		len = Pack_MsgD2SUserDataRequest(req_msg_buf, &req_msg);
		Msg_Cmd_Add_queue(req_msg_buf, len);
	    }
	}

	/* 
	 * 等待请求的对讲音频数据接收完毕后进行播放
	 */
	if (ayutil_wait_sem(&ptalk_ctrl->play_ok, 10*1000) == 0)
	{
	    ptalk_cb = sdk_get_handle(0)->cbfuncs.pAudioCbfc;
	    if ( ptalk_cb != NULL)
	    {
		ptalk_ctrl->ulk_buf.audio_type = tmp_info->type;
		ptalk_ctrl->ulk_buf.audio_buf = ptalk_buf->buf;
		ptalk_ctrl->ulk_buf.audio_len = ptalk_buf->len;
		ptalk_ctrl->ulk_buf.audio_seq = ptalk_buf->seq;
		ptalk_ctrl->ulk_buf.is_end = 1;

		DEBUGF("--------------->callback audio_seq = %d\n", ptalk_buf->seq);
		(*ptalk_cb)(&ptalk_ctrl->ulk_buf);
	    }
	}

	ptalk_buf->len = 0;
	ptalk_ctrl->flag = 1;// restore next talk continue 
	Audio_msg_Del_queue_first();
    }

    sem_destroy(&ptalk_ctrl->play_ok);
    ptalk_ctrl->flag = 0;
    ptalk_buf->len = 0;
}

int  copy_talk_data(uint32  audio_total_len,uint32  audio_offset,uint32  audio_len,uint8 *audio_data)
{
    Audio_Talk_Buf_Struct *ptalk_buf = &sdk_get_handle(0)->talk.talk_buf;
    Audio_Talk_Ctrl_Struct *ptalk_ctrl = &sdk_get_handle(0)->talk.talk_ctrl;

    if ( audio_total_len > sizeof(ptalk_buf->buf) )
    {
	ERRORF("talk data len too long:%d\n",audio_total_len);
	return -1;
    }

    int cnt = 40;
    while(ptalk_ctrl->flag==2 && --cnt)
    {
	usleep(5000);
    }
    if(cnt <= 0)
    {
	ERRORF("last talk not end,flag = %d\n",ptalk_ctrl->flag);
	return -1;
    }

    if (audio_len + ptalk_buf->len <= sizeof(ptalk_buf->buf))
    {
	memcpy((ptalk_buf->buf + audio_offset), audio_data, audio_len);
	ptalk_buf->len += audio_len;
    }
    else
    {
	ptalk_ctrl->seq_cur = 0;
	ptalk_buf->len = 0;
	return -1;
    }
    if(ptalk_buf->len == audio_total_len)
    {
	ptalk_ctrl->flag = 2;// wait to callback handle,MUST before @sem_post
	sem_post(&ptalk_ctrl->play_ok);
    }

    return 0;
}

int  History_List_Upload(HistoryListQuery_Struct *his_list_report, uint32 flag)
{
    unsigned int 		rlst = 0;
    char   			sub_p_buf[1024+128];
    MsgD2SNVRHistoryListNotify  his_list;

    if ( his_list_report->channel_index< 1 || his_list_report->channel_index > MAX_CHANNEL_NUM )
    {
        return AY_ERROR_CHN;
    }

    if(aystream_get_status() > 0)
    {
	his_list.mask=0x0001;
	his_list.flag=flag;
	memcpy(&his_list.did,&sdk_get_handle(0)->devinfo.did,sizeof(device_id_t));
	his_list.channel_index=his_list_report->channel_index;
	his_list.rate=his_list_report->rate;

	his_list.mask|=0x0002;
	his_list.start_time=his_list_report->start_time;
	his_list.end_time=his_list_report->end_time;

	his_list.mask|=0x0004;
	memcpy(&his_list.hList,&his_list_report->hList,sizeof(NVR_history_block)*his_list_report->list_len);
	his_list.list_len=his_list_report->list_len;
	his_list.seq=his_list_report->seq;

	int i = 0;
	for(i=0;i<his_list.list_len;i++)
	{
	    //DEBUGF("list[%d] start time = %u, end time = %u\n",i,his_list.hList[i].start_time,his_list.hList[i].end_time);
	}
	rlst = Pack_MsgD2SNVRHistoryListNotify(sub_p_buf, &his_list);
	DEBUGF("History_List_Upload start = %u, end = %u, mask = %d,list len = %d,rlst = %d\n", 
		his_list.start_time, his_list.end_time, his_list.mask,his_list.list_len,rlst);
	
	Msg_Cmd_Add_queue (sub_p_buf, rlst);

        return  AY_ERROR_OK;
    }
    return AY_ERROR_NETWORK;
}


int History_Srch_Rslt_Ack(uint32 ts_size,uint8 chn)
{
    char   tempbuf[300];
    int    len;
    ULK_Hist_Rslt_Ack	*hist_Rslt;
    st_ay_history_ctrl *phist = &sdk_get_handle(0)->history;
    
    if(chn < 1 || chn >= MAX_CHANNEL_NUM)
    {
	return -1;
    }

    hist_Rslt = &phist->his_ack[chn-1];
    hist_Rslt->ts_size = ts_size;

    MsgD2STSDataInfoNotify  search_rslt;
    search_rslt.mask = 0x01;
    search_rslt.channel_index 	= hist_Rslt->channel_index;
    search_rslt.rate 		= hist_Rslt->rate;
    search_rslt.ts_ts 		= hist_Rslt->ts_ts;
    search_rslt.mask |= 0x02;
    search_rslt.ts_size 	= hist_Rslt->ts_size;
    search_rslt.ts_duration 	= hist_Rslt->ts_duration;

    if ( hist_Rslt->ts_size > 0 )
    {
	search_rslt.flag = 0x01;
    }
    if(phist->his_info[chn-1].his_flag==0)
        search_rslt.flag |= 0x02;
    else if(phist->his_info[chn-1].his_flag==1)
        search_rslt.flag |= 0x04;

    search_rslt.mask |= 0x08;
    search_rslt.req_id_length   = phist->his_info[chn-1].his_req_id_length;
    search_rslt.request_id      = phist->his_info[chn-1].his_request_id;

    len = Pack_MsgD2STSDataInfoNotify(tempbuf, &search_rslt);
    DEBUGF("search:size=%d,durtn=%d\n",search_rslt.ts_size, search_rslt.ts_duration);

    Msg_Cmd_Add_queue(tempbuf, len);
    return 0;
}

void upload_screen_snap(int arg)
{
    unsigned int i, tmp, rlst = 0;
    char   	sub_p_buf[1024+128];
    Screen_Shot_Struct *ppic = (Screen_Shot_Struct*)arg;
    uint32 flag = enm_type_instant;

    if(NULL == ppic)
	return ;
    MsgD2SScreenShotNotify  sub_pic_pack;
    
    sub_pic_pack.mask = 0x0001;
    sub_pic_pack.flag = flag;
    sub_pic_pack.pic_MTU = SCREENSHOT_SUB_SIZE;
    sub_pic_pack.channel_index = ppic->channel_index;
    sub_pic_pack.update_rate =  ppic->update_rate;
    sub_pic_pack.status = ppic->alarm_type;
    sub_pic_pack.screenshot_ts = ppic->screenshot_ts;
    sub_pic_pack.screenshot_size = ppic->pic_length;
    tmp = ppic->pic_length / SCREENSHOT_SUB_SIZE;
    for( i = 0; i < tmp; i++)
    {
	sub_pic_pack.mask |= 0x0002;
	sub_pic_pack.offset = i * SCREENSHOT_SUB_SIZE;
	sub_pic_pack.length = SCREENSHOT_SUB_SIZE;
	sub_pic_pack.data = ppic->pic_data + sub_pic_pack.offset;
	rlst = Pack_MsgD2SScreenShotNotify(sub_p_buf, &sub_pic_pack);
	while(!sdk_get_handle(0)->exit_flag)
	{
	    if(add_msg_cmd(sub_p_buf, rlst,ADD_MSG_FLAG_FHINT) != AY_ERROR_MSGFULL)
		break;
	    usleep(100000);
	}
    }
    tmp = ppic->pic_length % SCREENSHOT_SUB_SIZE;
    if (tmp > 0)
    {
	sub_pic_pack.mask |= 0x0002;
	sub_pic_pack.offset = i * SCREENSHOT_SUB_SIZE;
	sub_pic_pack.length = tmp;
	sub_pic_pack.data = ppic->pic_data + sub_pic_pack.offset;
	rlst = Pack_MsgD2SScreenShotNotify(sub_p_buf, &sub_pic_pack);
	while(!sdk_get_handle(0)->exit_flag)
	{
	    if(add_msg_cmd(sub_p_buf, rlst,ADD_MSG_FLAG_FHINT) != AY_ERROR_MSGFULL)
		break;
	    usleep(100000);
	}
    }

    free(ppic);
    ppic = NULL;
    sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_SNAP;
}

int is_chn_filter(Chnl_ctrl_table_struct *ptable,int chn,int frm_type,int brate)
{
    int filter = 0;
    if(IS_LIVE_FRAME(frm_type))
    {
	int index = ayutil_query_rate_index(brate);
	if(ptable[chn - 1].bit_r[index] == 0 )
	{
	    filter = 1;
	}
	else if(frm_type==CH_AUDIO_FRM) 
	{
	    if(!(ptable[chn - 1].bit_r[index]&0x02))
	    { 
		filter = 1;
	    }
	}
	else if(!(ptable[chn - 1].bit_r[index]&0x01))
	{// this shouldnot happen
	    filter = 1;
	}
    }
    else
    {
	if(ptable[chn - 1].bit_his_r == 0)
	{
	    filter = 1;
	}
	else if(frm_type==CH_HIS_AUDIO_FRM && !(ptable[chn - 1].bit_his_r&0x02))
	{
	    filter = 1;
	}
    }
    return filter;
}

void update_send_I_ts(Chnl_ctrl_table_struct *ptable,int chn,int rate,uint32 ts)
{
    int index = ayutil_query_rate_index(rate);

    ptable[chn - 1].frm_id[index].now_send_I_ts = ts;
    //printf("$$$$$$$$$$$$$[sdk]update send I ts,chn = %d,rate = %d,ts = %u\n",chn,rate,ptable[chn - 1].frm_id[index].now_send_I_ts);
}

int compare_send_I_ts(Chnl_ctrl_table_struct *ptable,int chn,int rate,uint32 ts)
{
    int index = ayutil_query_rate_index(rate);

    if(ptable[chn - 1].frm_id[index].now_send_I_ts == 0)
	return 0;
    if(ptable[chn - 1].frm_id[index].latest_gop_gap == 0)
	return 0;

    if(ts > ptable[chn - 1].frm_id[index].now_send_I_ts + ptable[chn - 1].frm_id[index].latest_gop_gap*2)
    {
	//printf("$$$$$$$$$$$$$[sdk]compare send I ts,chn = %d,rate = %d,now = %ld,ts = %u\n",
	//	chn,rate,ts,ptable[chn - 1].frm_id[index].now_send_I_ts);

	ptable[chn - 1].frm_id[index].now_send_I_ts = 0; // reset it,wait next update
	return 1;
    }
    return 0;
}

int disable_stream_request(Chnl_ctrl_table_struct ptable[],int from)
{
    int   i;

    for ( i = 0 ; i < sdk_get_handle(0)->devinfo.channelnum; i++ )
    {
	if(ptable[i].bit_r[0]&&0x01)
	{
	    add_channel_cmd(VIDEO_CTRL,i+1,0,UPLOAD_RATE_1,from);
	}
	if(ptable[i].bit_r[0]&&0x02)
	{
	    add_channel_cmd(AUDIO_CTRL,i+1,0,UPLOAD_RATE_1,from);
	}
	if(ptable[i].bit_r[1]&&0x01)
	{
	    add_channel_cmd(VIDEO_CTRL,i+1,0,UPLOAD_RATE_2,from);
	}
	if(ptable[i].bit_r[1]&&0x02)
	{
	    add_channel_cmd(AUDIO_CTRL,i+1,0,UPLOAD_RATE_2,from);
	}
	if(ptable[i].bit_r[2]&&0x01)
	{
	    add_channel_cmd(VIDEO_CTRL,i+1,0,UPLOAD_RATE_3,from);
	}
	if(ptable[i].bit_r[2]&&0x02)
	{
	    add_channel_cmd(AUDIO_CTRL,i+1,0,UPLOAD_RATE_3,from);
	}
	if(ptable[i].bit_r[3]&&0x01)
	{
	    add_channel_cmd(VIDEO_CTRL,i+1,0,UPLOAD_RATE_4,from);
	}
	if(ptable[i].bit_r[3]&&0x02)
	{
	    add_channel_cmd(AUDIO_CTRL,i+1,0,UPLOAD_RATE_4,from);
	}
	
	if(ptable[i].bit_his_r)
	{
	    add_channel_cmd(HISTORY_STOP,i+1,0,0,from);
	}
    }
    memset(ptable,0,sizeof(Chnl_ctrl_table_struct)*MAX_CHANNEL_NUM);
    return 0;
}

