#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol_device.h"
#include "aystream.h"
#include "ayqueue.h"
#include "aylan.h"
#include "define.h"
#include "ay_sdk.h"
#include "cdiffie_hell_man.h"
#include "http_base64.h"
#include "threadpool.h"

static inline uint16 fill_mask_and_flag(char *buf,uint32 mask,uint32 flag)
{
    *buf =(char)( mask);
    *(buf+1) = (char)(mask>>8);
    *(buf+2) =(char) (mask>>16);
    *(buf+3) =(char) (mask>>24);

    *(buf+4) =(char)( flag);
    *(buf+5) = (char)(flag>>8);
    *(buf+6) =(char) (flag>>16);
    *(buf+7) =(char) (flag>>24);
    return 8;
}

static inline uint16 fill_length_and_cmd(char *buf,uint16 len,uint32 cmd)
{
    *buf = (char)(len);
    *(buf + 1) = (char)(len>> 8);

    *(buf + 2) = (char)cmd;
    *(buf + 3) = (char)(cmd>> 8);
    *(buf + 4) = (char)(cmd>> 16);
    *(buf + 5) = (char)(cmd>> 24);
    return len;
}

uint8 *Check_ID_Error(uint8 *buf, int len, uint32 *err)
{
    uint8 *pos;
    char tmp;

    pos = (uint8 *)buf;

    tmp = *pos;// device_id_t: device_id_length(uint8)
    if (tmp <= 21)
    {
        pos += tmp + 1;
        *err = READ_4BYTE(pos);// c3_error_t: errcode(uint32)

        if (*err == 0)   /* 正常 */
        {
            pos += 4;
            pos += 1;   /* 跳过错误描述: '\0' */
            return pos;
        }
        else
        {
            ERRORF("msg ret err=%d\n", (int)*err);
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

int Pack_MsgDeviceRegisterRequest(char *buf, MsgDeviceRegisterRequest *msg)
{
    char *ptemp;
    uint16   templen;
    if (buf == NULL || msg== NULL)
    {
        return  -1;
    }

    DEBUGF("mask=0x%x,flag=0x%x\n", msg->mask, msg->flag);
    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if((msg->mask & 0x0001 ))
    {
        *ptemp++ =  msg->did.device_id_length;
        memcpy(ptemp, msg->did.device_id, msg->did.device_id_length);
        ptemp += msg->did.device_id_length;

        memcpy(ptemp, &msg->status_mask, sizeof(msg->status_mask));
        ptemp += sizeof(msg->status_mask);
    }
    if(msg->mask & 0x0002)
    {
	memcpy(ptemp, &msg->stream_serv_ip, sizeof(msg->stream_serv_ip));
	ptemp += sizeof(msg->stream_serv_ip);

	memcpy(ptemp, &msg->stream_serv_port, sizeof(msg->stream_serv_port));
	ptemp += sizeof(msg->stream_serv_port);

	memcpy(ptemp, &msg->token.token_bin_length, sizeof(msg->token.token_bin_length));
	ptemp += sizeof(msg->token.token_bin_length);

	memcpy(ptemp, msg->token.token_bin, msg->token.token_bin_length);
	ptemp += msg->token.token_bin_length;
    }
    if(msg->mask & 0x0004)
    {
	memcpy(ptemp, &msg->vs, sizeof(msg->vs));
	ptemp += sizeof(msg->vs);

	memcpy(ptemp, &msg->dev_type, sizeof(msg->dev_type));
	ptemp += sizeof(msg->dev_type);

	memcpy(ptemp, &msg->video_type, sizeof(msg->video_type));
	ptemp += sizeof(msg->video_type);

	memcpy(ptemp, &msg->channel_num, sizeof(msg->channel_num));
	ptemp += sizeof(msg->channel_num);

	memcpy(ptemp, &msg->min_rate, sizeof(msg->min_rate));
	ptemp += sizeof(msg->min_rate);

	memcpy(ptemp, &msg->max_rate, sizeof(msg->max_rate));
	ptemp += sizeof(msg->max_rate);
    }

    if(msg->mask & 0x0008)
    {
	memcpy(ptemp, &msg->hi_private_addr.IP, sizeof(msg->hi_private_addr.IP));
	ptemp += sizeof(msg->hi_private_addr.IP);

	memcpy(ptemp, &msg->hi_private_addr.Port, sizeof(msg->hi_private_addr.Port));
	ptemp += sizeof(msg->hi_private_addr.Port);
    }
    if(msg->mask & 0x0010)  /* 入口 */
    {
	memcpy(ptemp, &msg->reg_token.token_bin_length, sizeof(msg->reg_token.token_bin_length));
	ptemp += sizeof(msg->reg_token.token_bin_length);

	memcpy(ptemp, msg->reg_token.token_bin, msg->reg_token.token_bin_length);
	ptemp += msg->reg_token.token_bin_length;
    }
    if(msg->mask & 0x0020)  /* 目标ip */
    {
	memcpy(ptemp, &msg->entry_ip, sizeof(msg->entry_ip));
	ptemp += sizeof(msg->entry_ip);
    }
    if(msg->mask & 0x0040)  /* 图像宽度 */
    {
	memcpy(ptemp, &msg->video_width, sizeof(msg->video_width));
	ptemp += sizeof(msg->video_width);

	memcpy(ptemp, &msg->video_height, msg->video_height);
	ptemp += sizeof(msg->video_height);
    }
    if(msg->mask & 0x0080)  /* 通道状态 */
    {
	memcpy(ptemp, &msg->channel_mask_len, sizeof(msg->channel_mask_len));
	ptemp += sizeof(msg->channel_mask_len);

	memcpy(ptemp, msg->channel_mask, msg->channel_mask_len);
	ptemp += msg->channel_mask_len;

	DEBUGF("channel mask len=%d,mask[0] %x mask[1] =%x\n", msg->channel_mask_len, msg->channel_mask[0], msg->channel_mask[1]);
    }
    if(msg->mask & 0x0100)// rtsp url
    {
	memcpy(ptemp, msg->rtsp_url, strlen(msg->rtsp_url)+1);
	ptemp += strlen(msg->rtsp_url)+1;

	DEBUGF("rtsp_url =%s\n", msg->rtsp_url);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_DeviceRegisterRequest);

    return templen;
}

int Pack_MsgDeviceLoginRequest(char *buf, MsgDeviceLoginRequest *msg)
{
    char 	*ptemp;
    uint16   templen;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    if(buf == NULL || msg== NULL)
    {
	return  -1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if((msg->mask & 0x0001 ))
    {
	*ptemp++ =  msg->did.device_id_length;
	memcpy(ptemp, msg->did.device_id, msg->did.device_id_length);
	ptemp += msg->did.device_id_length;

	*ptemp++ =  (char)(pdevinfo->token.token_bin_length);
	*ptemp++ =  (char)(pdevinfo->token.token_bin_length >> 8);

	memcpy(ptemp, pdevinfo->token.token_bin, pdevinfo->token.token_bin_length);
	ptemp += pdevinfo->token.token_bin_length;

	memcpy(ptemp, &msg->vs, sizeof(msg->vs));
	ptemp += sizeof(msg->vs);

	memcpy(ptemp, &msg->status_mask, sizeof(msg->status_mask));
	ptemp += sizeof(msg->status_mask);

	memcpy(ptemp, &msg->dev_type, sizeof(msg->dev_type));
	ptemp += sizeof(msg->dev_type);

	memcpy(ptemp, &msg->video_type, sizeof(msg->video_type));
	ptemp += sizeof(msg->video_type);

	memcpy(ptemp, &msg->channel_num, sizeof(msg->channel_num));
	ptemp += sizeof(msg->channel_num);

	memcpy(ptemp, &msg->min_rate, sizeof(msg->min_rate));
	ptemp += sizeof(msg->min_rate);

	memcpy(ptemp, &msg->max_rate, sizeof(msg->max_rate));
	ptemp += sizeof(msg->max_rate);
    }
    if(msg->mask & 0x0002)
    {
	memcpy(ptemp, &(msg->hi_private_addr.IP), sizeof(msg->hi_private_addr.IP));
	ptemp += sizeof(msg->hi_private_addr.IP);
	memcpy(ptemp, &(msg->hi_private_addr.Port), sizeof(msg->hi_private_addr.Port));
	ptemp += sizeof(msg->hi_private_addr.Port);
    }
    if(msg->mask & 0x0004)
    {
	memcpy(ptemp, &msg->current_time, sizeof(msg->current_time));
	ptemp += sizeof(msg->current_time);
	memcpy(ptemp, &msg->current_tick, sizeof(msg->current_tick));
	ptemp += sizeof(msg->current_tick);
    }
    if(msg->mask & 0x0008)
    {
	*ptemp = msg->frame_rate;
	ptemp++;
    }

    if(msg->mask & 0x0010)
    {
	memcpy(ptemp, &msg->uchannelcount, sizeof(msg->uchannelcount));
	ptemp += sizeof(msg->uchannelcount);

	memcpy(ptemp, &msg->uSamplerate, sizeof(msg->uSamplerate));
	ptemp += sizeof(msg->uSamplerate);
	memcpy(ptemp, &msg->ubitLength, sizeof(msg->ubitLength));
	ptemp += sizeof(msg->ubitLength);
    }
    if(msg->mask & 0x0020)
    {	    
	memcpy(ptemp, &msg->channel_mask_len, sizeof(msg->channel_mask_len));
	ptemp += sizeof(msg->channel_mask_len);

	memcpy(ptemp, msg->channel_mask, msg->channel_mask_len);
	ptemp += msg->channel_mask_len;
	//DEBUGF("=====>> channel mask len=%d,msg->channel_mask[0] %x\n", msg->channel_mask_len, msg->channel_mask[0]);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_DeviceLoginRequest);

    return templen;
}

int Pack_MsgStreamFrameInfoNofity(char *buf, MsgStreamFrameInfoNofity *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return  -1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if((msg->mask & 0x0001 ))
    {
	*ptemp++ =  msg->channel_index;

	SERIALIZE_WORD(msg->update_rate,ptemp);
	SERIALIZE_DWORD(msg->frm_seq,ptemp);
	SERIALIZE_DWORD(msg->frm_ts,ptemp);
	SERIALIZE_DWORD(msg->frm_size,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	SERIALIZE_DWORD(msg->crc32_hash,ptemp);
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->datalen,ptemp);

	memcpy(ptemp, msg->pdata, msg->datalen);
	ptemp += msg->datalen;
    }

    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->av_frm_seq,ptemp);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_StreamFrameInfoNofity);

    return templen;
}

static int __Pack_MsgStreamFrameDataResponse(char *buf, uint32 cid,MsgStreamFrameDataResponse *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {
	*ptemp++ =	msg->channel_index;

	SERIALIZE_WORD(msg->update_rate,ptemp);
	SERIALIZE_DWORD(msg->frm_seq,ptemp);
	SERIALIZE_DWORD(msg->offset,ptemp);
	SERIALIZE_DWORD(msg->length,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	memcpy(ptemp, msg->data, msg->length);
	ptemp += msg->length;
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->frm_ts,ptemp);
	SERIALIZE_DWORD(msg->frm_size,ptemp);
    }

    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->av_frm_seq,ptemp);
    }

    if(msg->mask & 0x0010)
    {
	SERIALIZE_DWORD(msg->frm_time,ptemp);
    }
    if(msg->mask & 0x0020)
    {
	*ptemp++ = (char)( msg->device_hash[0]);
	*ptemp++ = (char)( msg->device_hash[1]);
    }
    if (msg->mask & 0x0040)
    {
	SERIALIZE_DWORD(msg->alarm_type, ptemp);
	SERIALIZE_DWORD(msg->alarm_time, ptemp);
    }
    if (msg->mask & 0x0080)
    {
	SERIALIZE_DWORD(msg->media_video_type, ptemp);
	SERIALIZE_DWORD(msg->media_audio_type, ptemp);
	*ptemp++ = (char)( msg->reserved);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,cid);

    return templen;
}
int Pack_MsgStreamFrameDataResponse(char *buf, MsgStreamFrameDataResponse *msg)
{
    return __Pack_MsgStreamFrameDataResponse(buf,CID_StreamFrameDataResponse,msg);
}
int Pack_MsgD2CStreamFrameDataResponse(char *buf, MsgStreamFrameDataResponse *msg)
{
    return __Pack_MsgStreamFrameDataResponse(buf,CID_D2CStreamFrameDataResponse,msg);
}

static int __Pack_MsgStatusReport(char *buf, uint32 cid,MsgStatusReport *msg)
{
    char *ptemp;
    uint16 templen;
    uint16 i;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {
	SERIALIZE_DWORD(msg->status_mask,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	SERIALIZE_WORD(msg->min_rate,ptemp);
	SERIALIZE_WORD(msg->max_rate,ptemp);
    }
    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->stream_serv_ip,ptemp);
	SERIALIZE_WORD(msg->stream_serv_port,ptemp);

	SERIALIZE_WORD(msg->new_token.token_bin_length,ptemp);
	for(i=0; i<msg->new_token.token_bin_length; i++ )
	{
	    *ptemp++ = (char)msg->new_token.token_bin[i];
	}
    }
    if ( msg->mask & 0x0010 )
    {
	memcpy(ptemp, &msg->channel_mask_len, sizeof(msg->channel_mask_len));
	ptemp += sizeof(msg->channel_mask_len);

	memcpy(ptemp, msg->channel_mask, msg->channel_mask_len);
	ptemp += msg->channel_mask_len;

	//DEBUGF("channel mask len=%d,msg->channel_mask[0] %x\n", msg->channel_mask_len, msg->channel_mask[0]);
    }
    if ( msg->mask & 0x0020 )
    {
	SERIALIZE_WORD(msg->oem_data_len,ptemp);
	memcpy(ptemp, msg->oem_data, msg->oem_data_len);
	ptemp += msg->oem_data_len;  
    }

    if ( msg->mask & 0x0040 )
    {
	SERIALIZE_WORD(msg->channel_status_num,ptemp);
	memcpy(ptemp, msg->channels_status, msg->channel_status_num*sizeof(Channel_Status));
	ptemp += msg->channel_status_num*sizeof(Channel_Status);
    }
    if(msg->mask & 0x0080)
    {
	SERIALIZE_DWORD(msg->channel_q_num,ptemp);
	SERIALIZE_DWORD(msg->report_rslt,ptemp);
	SERIALIZE_DWORD(msg->callback_status,ptemp);
    }
    if(msg->mask & 0x0100)
    {
	*ptemp++ = msg->channel_index_zoom;
	SERIALIZE_DWORD(msg->zoom_value,ptemp);
	DEBUGF("0x0100...zoom chn = %d,val = %d\n",msg->channel_index_zoom,msg->zoom_value);
    }
    if(msg->mask & 0x0200)
    {
	*ptemp++ = msg->sdcard_status;
	*ptemp++ = msg->record_mode;
	DEBUGF("0x0200...record sd = %d,mod = %d\n",msg->sdcard_status,msg->record_mode);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,cid);

    return templen; 
}
int Pack_MsgStatusReport(char *buf, MsgStatusReport *msg)
{
    return __Pack_MsgStatusReport(buf,CID_StatusReport,msg);
}
int Pack_MsgD2CStatusReport(char *buf, MsgStatusReport *msg)
{
    return __Pack_MsgStatusReport(buf,CID_D2CStatusReport,msg);
}

int Pack_MsgD2STSDataInfoNotify(char *buf, MsgD2STSDataInfoNotify *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {
	*ptemp++ = (char)( msg->channel_index);

	SERIALIZE_WORD(msg->rate,ptemp);
	SERIALIZE_DWORD(msg->ts_ts,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	SERIALIZE_DWORD(msg->ts_size,ptemp);
	SERIALIZE_DWORD(msg->ts_duration,ptemp);
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->length,ptemp);

	memcpy(ptemp, msg->ts_data, msg->length);
	ptemp += msg->length;
    }
    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->req_id_length,ptemp);
	memcpy(ptemp, msg->request_id, msg->req_id_length);
	ptemp += msg->req_id_length;
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2STSDataInfoNotify);

    return templen;
}

int Pack_MsgD2STSDataResponse(char *buf, MsgD2STSDataResponse *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {
	*ptemp++ = (char)( msg->channel_index);

	SERIALIZE_WORD(msg->rate,ptemp);
	SERIALIZE_DWORD(msg->ts_ts,ptemp);
	SERIALIZE_DWORD(msg->offset,ptemp);
	SERIALIZE_DWORD(msg->length,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	memcpy(ptemp, msg->ts_data, msg->length);
	ptemp += msg->length;
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->ts_size,ptemp);
	SERIALIZE_DWORD(msg->ts_duration,ptemp);
    }

    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->req_id_length,ptemp);
	memcpy(ptemp, msg->request_id, msg->req_id_length);
	ptemp += msg->req_id_length;		
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2STSDataResponse);

    return templen;
}

int Pack_MsgD2SUserDataRequest(char *buf, MsgD2SUserDataRequest *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {
	strcpy(ptemp,  msg->user_name);
	ptemp += strlen(msg->user_name)+1;

	SERIALIZE_DWORD(msg->seq,ptemp);
	SERIALIZE_DWORD(msg->audio_offset,ptemp);
	SERIALIZE_DWORD(msg->audio_len,ptemp);

    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2SUserDataRequest);

    return templen;
}


int Pack_MsgD2SScreenShotNotify(char *buf, MsgD2SScreenShotNotify *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {	
	SERIALIZE_WORD(msg->pic_MTU,ptemp);
	*ptemp++ = (char)( msg->channel_index);
	SERIALIZE_WORD(msg->update_rate,ptemp);

	SERIALIZE_DWORD(msg->status,ptemp);
	SERIALIZE_DWORD(msg->screenshot_ts,ptemp);
	SERIALIZE_DWORD(msg->screenshot_size,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	SERIALIZE_DWORD(msg->offset,ptemp);
	SERIALIZE_DWORD(msg->length,ptemp);
	memcpy(ptemp, msg->data, msg->length);
	ptemp += msg->length;		
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->crc32_hash,ptemp);
    }
    if(msg->mask & 0x0008)
    {
	SERIALIZE_DWORD(msg->img_type_len,ptemp);

	memcpy(ptemp, msg->img_type, msg->img_type_len);
	ptemp += msg->img_type_len;
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2SScreenShotNotify);

    return templen;
}

int Pack_MsgD2SNVRHistoryListNotify(char *buf, MsgD2SNVRHistoryListNotify *msg)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return  -1;
    }
    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if(msg->mask & 0x0001)
    {	
	*ptemp++ = (char)msg->channel_index;
	SERIALIZE_WORD(msg->rate,ptemp);
    }
    if(msg->mask & 0x0002)
    {
	SERIALIZE_DWORD(msg->start_time,ptemp);
	SERIALIZE_DWORD(msg->end_time,ptemp);
    }
    if(msg->mask & 0x0004)
    {
	SERIALIZE_DWORD(msg->list_len,ptemp);
	if(msg->list_len!=0)
	{
	    memcpy(ptemp,msg->hList,sizeof(NVR_history_block)*msg->list_len);
	    ptemp=ptemp+sizeof(NVR_history_block)*(msg->list_len);
	}
	SERIALIZE_DWORD(msg->seq,ptemp);
    }
    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2SNVRHistoryListNotify);

    return templen;
}

int Unpack_MsgDeviceRegisterRespons(char *buf, uint16 len)
{
    if (buf == NULL || len < 8) {
        return	-1;
    }

    int ret = 0;
    uint8 *pos = (uint8 *)buf;
    uint32   mask, flag, temp2, errcode;
    uint16   temp16;
    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    DEBUGF("mask=0x%x, flag=0x%x\n", mask, flag);

    //aydebug_printf("regist ack", buf, len);
    if (mask & 0x0001)
    {
        DEBUGF("0x0001...\n");
        pos = Check_ID_Error(pos, len, &errcode);
        if ( errcode == -1 )
        {
            ERRORF("entry token not available\n");
            return AY_ERROR_TOKEN;
        }
        else if (errcode == -2)//stream server unavailable
        {
            ERRORF("errcode = -2, clear strmserv and token!\n");
            //pdevinfo->stream_serv_ip = 0;
            //pdevinfo->stream_serv_port = 0;
            //pdevinfo->token.token_bin_length = 0;
            //memset(pdevinfo->token.token_bin, 0, sizeof(pdevinfo->token.token_bin));
        }

        if (pos == NULL)
        {     
            return -1;
        }

        temp16 = READ_2BYTE(pos);
        if ((temp16 >= 3) && (temp16 <= 600))
        {
            pdevinfo->expected_cycle = temp16;
            DEBUGF("entry expected cycle=%d\n", temp16);
        }
        pos += 2;
    }

    if (mask & 0x0002)//public ip info
    {
        DEBUGF("0x0002...\n");
        pdevinfo->public_net_ip = READ_4BYTE(pos);
        pos += 4;
        pdevinfo->public_net_port = READ_2BYTE(pos);
        pos += 2;
    }
    if (mask & 0x0004)//stream server info
    {
        DEBUGF("0x0004... stream server info\n");

        uint32 stream_serv_ip;
        uint16 stream_serv_port;
        uint16 token_len;

        stream_serv_ip = READ_4BYTE(pos);
        pos += 4;
        stream_serv_port = READ_2BYTE(pos);
        pos += 2;
        token_len = READ_2BYTE(pos);
        pos += 2;

        if(aystream_get_status()<0 || (flag&0x0001))
        {// Firstly if don't connect 
            TRACEF("{{CmdEvent}}%s|ForceReconnect|flag = %#x,token length = %d\n",
            pdevinfo->now_ack_tracker_addr,flag,token_len);
            memcpy(pdevinfo->token.token_bin, pos, token_len);
            pdevinfo->token.token_bin_length = token_len;
            pdevinfo->stream_serv_port = stream_serv_port; 
            pdevinfo->stream_serv_ip = stream_serv_ip;  // we keep ip changed at last
        }
        else if((pdevinfo->stream_serv_ip == stream_serv_ip) && (pdevinfo->stream_serv_port == stream_serv_port))
        {// Secondly if has connected, we just handle token-update request.
            if(pdevinfo->token.token_bin_length==0 || memcmp(pdevinfo->token.token_bin, pos, token_len)!=0)
            {
                TRACEF("{{CmdEvent}}%s|TokenUpdate|token len=%d,ip=%u, port=%u\n",
                    pdevinfo->now_ack_tracker_addr,token_len, stream_serv_ip, stream_serv_port);
                memcpy(pdevinfo->token.token_bin, pos, token_len);
                pdevinfo->token.token_bin_length = token_len;
                pdevinfo->token_update_flag = 1;
            }
        }
        pos += token_len;
        ret = AY_ERROR_STREAM;// we have recv streamip info
    }
    if(mask & 0x0008)
    {}
    if(mask & 0x0010)
    {
        CMD_PARAM_STRUCT ts_cmd;
        memset(&ts_cmd,0,sizeof(ts_cmd));
        ts_cmd.cmd_id = TIME_SYN;
        ts_cmd.cmd_args_len = 4;
        memcpy(ts_cmd.cmd_args,pos,4);

        DEBUGF("0x0010\n");
        temp2 = READ_4BYTE(pos);
        pos += 4;

        DEBUGF("[TIME_SYN]update systime =%d\n", temp2);
        Msg_Cmd_Add_down_queue((char*)&ts_cmd,sizeof(CMD_PARAM_STRUCT));
    }

    if(mask & 0x0020)
    {/* upgrade */
        version_t new_ver;
        memcpy(&new_ver, pos, sizeof(version_t));
        pos += sizeof(version_t);

        temp2 = READ_4BYTE(pos);
        pos += 4;
        DEBUGF("ver: %u:%u:%u:%u,crc32:%u,url:%s\n",new_ver[0],new_ver[1],new_ver[2],new_ver[3], temp2, pos);
        //update_self((char *)pos, new_ver, temp2);
    }

    return ret;
}

int Unpack_MsgDeviceLoginResponse(char *buf, uint16 len)
{
    if (buf == NULL || len < 8)
    {
        return	-1;
    }

    uint8 *pos = (uint8 *)buf;
    uint32 mask, flag, err;
    uint16 upload_rate;
    uint8 chno, enable_upload;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    DEBUGF("mask=0x%x,flag=0x%x\n", mask, flag);

    if (mask & 0x0001)
    {
        DEBUGF("0x0001...\n");
        pos = Check_ID_Error(pos, len, &err);
        if (err == -3)
        {
            ERRORF("stream login token Invalid,reset it!\n");
            sdk_get_handle(0)->devinfo.token.token_bin_length = 0;
            return AY_ERROR_TOKEN;
        }
        if (pos == NULL)
        {
            return err;
        }
        pos += 4; //Public IP
    }

    if (mask & 0x0002)
    {
        DEBUGF("0x0002...\n");
        enable_upload = *pos++;
        upload_rate = READ_2BYTE(pos);

        DEBUGF("Unpack_MsgDeviceLoginResponse enable_upload = %d\n", enable_upload);

        pos += 2;
        chno = *pos++;

        if (flag & enm_enable_audio)
        {
            add_channel_cmd(AUDIO_CTRL, chno, enable_upload, (int)upload_rate, CMD_FROM_SERVER);
        }

        if (flag & enm_enable_video)
        {
            DEBUGF("server don't stop motion detect upload\n");
            /* 平台关闭设备推流的时候，不关闭移动侦测上传流 */
            /*if (sdk_get_handle(0)->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE && !enable_upload)
            {
                if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM)
                {
                    sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
                }
            }*/
            add_channel_cmd(VIDEO_CTRL, chno, enable_upload, (int)upload_rate, CMD_FROM_SERVER);
        }
    }

    return (uint16)(pos - pos);
}

int Unpack_MsgStreamFrameDataRequest(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len< 8)
    {
	return	-1;
    }

    uint8 *pos = (uint8 *)buf;
    uint32   mask, flag;
    uint16   temp16;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    flag = flag; /*去掉警告 */

    if(mask & 0x0001)
    {
	//DEBUGF("0x0001...\n");
	temp16 = READ_2BYTE(pos);
	if((temp16 >= 3) && (temp16 <= 60))
	{
	    sdk_get_handle(0)->devinfo.expt_cycle_strm = temp16;
	    DEBUGF("expt_cycle_strm=%d\n", temp16);
	}
	pos += 2;
    }

    return (uint16)(pos - pos);
}

int Unpack_MsgStatusReportRes(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len < 8)
    {
	return	-1;
    }

    sdk_get_handle(0)->stream.rcv_report_cnt[0]++;
    //sdk_get_handle(0)->stream.rcv_report_cnt[1]++;
    sdk_get_handle(0)->stream.rcv_report_cnt[1] = sdk_get_handle(0)->stream.snd_report_cnt[1];// reset

    CMD_PARAM_STRUCT   cmd;
    uint8 *pos = (uint8 *)buf;
    uint32  mask, flag;
    uint16  oem_data_len_t;
    uint8   *oem_data_t;
    int32   error_code_t; 

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    flag = flag;    /* 去掉警告 */

    //DEBUGF("mask=0x%x,flag=0x%x\n", mask, flag);
    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");
	sdk_get_handle(0)->devinfo.expt_cycle_strm = READ_4BYTE(pos); // FIXME
	DEBUGF("expt_cycle_strm=%d\n", sdk_get_handle(0)->devinfo.expt_cycle_strm);
	pos += 4;
    }
    if(mask & 0x0002)
    {
	DEBUGF("0x0002...\n");
	error_code_t = READ_4BYTE(pos);
	pos += 4;
	ERRORF("errcode=%d\n",error_code_t);
	pos += strlen((const char *)pos);
    }
    if(mask & 0x0004)
    {
	DEBUGF("0x0004...user data\n");
	oem_data_len_t = READ_2BYTE(pos);
	pos += 2;

	oem_data_t = pos;
	pos += oem_data_len_t;

	cmd.cmd_id = USER_DATA;
	cmd.cmd_args_len = oem_data_len_t;
	memcpy(cmd.cmd_args, oem_data_t, sizeof(cmd.cmd_args));
	Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));		
    }
    if(mask & 0x0008)
    {}

    return (uint16)(pos - pos);
}

int Unpack_MsgDeviceActionNofity(int skfd, char *buf, uint16 len)
{
    if (buf == NULL || len < 8)
    {
        return	-1;
    }
    char buf_sys[256];
    uint8 *pos = (uint8 *)buf;
    uint32 mask, flag, tmp;
    uint8 chno, enable_upload;
    uint16 upload_rate;
    uint32 act;
    uint8 act_chn = 0;

    struct in_addr ip;
    ip.s_addr = sdk_get_handle(0)->devinfo.stream_serv_ip;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    DEBUGF("mask=0x%x,flag=0x%x\n", mask, flag);

    if (mask & 0x0001)
    {
        DEBUGF("0x0001...\n");
        enable_upload = *pos;
        DEBUGF("Unpack_MsgDeviceActionNofity enable_upload = %d\n", enable_upload);
        upload_rate = READ_2BYTE(pos + 1);
        chno = *(pos + 3);

        if (flag & enm_control_record)
        {
            if ((flag & enm_enable_video) && (flag & enm_enable_audio) && enable_upload == 0)
            {
                add_channel_cmd(HISTORY_STOP, chno, enable_upload, upload_rate, CMD_FROM_SERVER);
            }
        }
        else if (flag & enm_control_record_extend)
        {
            uint32 hist_ctrl = 0;
            if (flag & enm_enable_video)
            {
                hist_ctrl |= enable_upload ? 0x01 : 0x00;// 0x01 - video, 0x02 - audio
                TRACEF("{{MediaEvent}}%d|%d|%s|Playback|Video|%s|control\n", chno, upload_rate, inet_ntoa(ip), enable_upload ? "Start" : "Stop");
            }
            if (flag & enm_enable_audio)
            {
                hist_ctrl |= enable_upload ? 0x02 : 0x00;// 0x01 - video, 0x02 - audio
                TRACEF("{{MediaEvent}}%d|%d|%s|Playback|Audio|%s|control\n", chno, upload_rate, inet_ntoa(ip), enable_upload ? "Start" : "Stop");
            }

            if (hist_ctrl)
            {
                sdk_get_handle(0)->devobj[0].streamFilter[chno-1].bit_his_r = hist_ctrl;
            }
        }
        else
        {// live stream control
            if (flag & enm_enable_audio)
            {
                add_channel_cmd(AUDIO_CTRL, chno, enable_upload, upload_rate, CMD_FROM_SERVER);
            }

            if (flag & enm_enable_video)
            {
                DEBUGF("server don't stop motion detect upload\n");
                /*if(sdk_get_handle(0)->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE && !enable_upload)
                {
                    if(sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM)
                    {
                        sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
                    }
                }*/
                add_channel_cmd(VIDEO_CTRL, chno, enable_upload, upload_rate, CMD_FROM_SERVER);
            }
        }
        pos += 4;
    }
    if (mask & 0x0002)
    {
        DEBUGF("msgAct ");
        act = READ_4BYTE(pos);
        pos += 4;
        act_chn = *pos;
        if (act == enm_server_reboot_device)
        {
            DEBUGF("reboot \n");
            //printf("-------------[todo] reboot in sdk -----------!\n");
            //system("reboot");
            PTZ_Channel_ctrl(act_chn, enm_server_reboot_device, 0);
        }
        else if (act == enm_server_reset_device)
        {
            DEBUGF("rm csp/* -rf\n");
            memset(buf_sys, 0, sizeof(buf_sys));
            snprintf(buf_sys, sizeof(buf_sys), "rm %s/csp/* -rf; reboot", sdk_get_handle(0)->devinfo.rw_path);
            //printf("------------- [todo]reset exec: %s  -----------!\n",buf_sys);
            //system(buf_sys);
            PTZ_Channel_ctrl(act_chn, enm_server_reset_device, 0);
        }
        else 
        {
            DEBUGF("ptz,chn = %d,cmd = %d\n", act_chn, act);
            if (!(mask&0x0010)) PTZ_Channel_ctrl(act_chn, act, 1); // default
        }
        pos += 1;
    }
    if ( mask & 0x0004 )
    {
        uint8 *from_user, *audio_data;
        uint32 audio_total_len, audio_offset, audio_len;

        DEBUGF("0x0004...audio talk\n");

        from_user = pos;
        tmp = strlen((char*)from_user) + 1;
        pos += tmp;
        pos += 4; /* audio seq */
        audio_total_len = READ_4BYTE(pos);
        pos += 4;
        audio_offset = READ_4BYTE(pos);
        pos += 4;
        audio_len = READ_4BYTE(pos);
        pos += 4;

        audio_data = pos;
        pos += audio_len;

        copy_talk_data(audio_total_len, audio_offset, audio_len, audio_data);
    }
    if (mask & 0x0008)
    {
        //uint16 expected_screenshot_cycle;  /* 期望的定时截图周期，以秒为单位，默认是10分钟 */
        pos += 2;
    }
    if (mask & 0x0010)
    {
        uint32 steps = READ_4BYTE(pos);
        DEBUGF("0x0010...ptz steps = %d\n", steps);
        PTZ_Channel_ctrl(act_chn, act, steps);
        pos += 4;
    }
    if (mask & 0x0020)
    {
        CMD_PARAM_STRUCT   cmd;
        cmd.channel =  1;
        cmd.cmd_id  = SD_CARD_CTRL;
        cmd.cmd_args[0] = *pos;
        cmd.cmd_args_len = 1;
        pos += 1;
        DEBUGF("0x0020...sd card ctrl,cmd = %d\n", cmd.cmd_args[0]);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }
    if (mask & 0x0040)
    {
        CMD_PARAM_STRUCT   cmd;
        cmd.channel =  1;
        cmd.cmd_id  = RECORD_CTRL;
        cmd.cmd_args[0] = *pos++;
        cmd.cmd_args[1] = *pos++;
        cmd.cmd_args_len = 2;
        DEBUGF("0x0040...record ctrl,cmd = %d,mode = %d\n",- cmd.cmd_args[0], cmd.cmd_args[1]);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }
    if (mask & 0x0080)
    {
        CMD_PARAM_STRUCT cmd;
        uint16 prepos = 0;
        act = *pos++;
        switch(act)
        {
            case enm_pp_noop: cmd.cmd_id = PTZ_SET_PRE_LOCAL; break;
            case enm_pp_set: cmd.cmd_id = PTZ_SET_PRE_LOCAL; break;
            case enm_pp_clear: cmd.cmd_id = PTZ_CLEAR_PRESET; break;
            case enm_pp_jump: cmd.cmd_id = PTZ_CALL_PRE_LOCAL; break;
        }
        cmd.channel = *pos++;
        prepos = READ_2BYTE(pos);
        cmd.cmd_args[0] = *pos++;
        cmd.cmd_args[1] = *pos++;
        cmd.cmd_args_len = 2;
        DEBUGF("0x0080...preset ctrl,cmd = %d,chn = %d,pos = %hu\n", cmd.cmd_id, cmd.channel, prepos);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }

    return (uint16)(pos - pos);
}

int Unpack_MsgS2DTSDataQuery(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len < 8)
    {
	return	-1;
    }

    struct in_addr ip;
    ip.s_addr = sdk_get_handle(0)->devinfo.stream_serv_ip;

    uint8 *pos = (uint8 *)buf;
    uint8 chn = 0;
    uint32 mask, flag, tmp2;
    st_ay_history_ctrl *phist = &sdk_get_handle(0)->history;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;
    flag = flag;

    DEBUGF("history request mask = %#x,flag = %#x\n",mask,flag);

    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");

	CMD_PARAM_STRUCT   cmd;
	uint32 start_tm, time_duration;
	uint16 brate = 0;

	cmd.cmd_id  = HISTORY_CTRL;
	cmd.channel = *pos++;
	cmd.cmd_args_len = 10;
	memcpy(cmd.cmd_args, pos, 10);// 2+4+4

	chn = cmd.channel - 1;
	brate = READ_2BYTE(pos);
	pos += 2;
	start_tm = READ_4BYTE(pos);	
	pos += 4;
	time_duration = READ_4BYTE(pos);
	pos += 4;
	DEBUGF("hist[%d] rate = %u, start =%u,duration=%u\n",cmd.channel,brate, start_tm, time_duration);
	TRACEF("{{MediaEvent}}%d|%d|%s|Playback|Video|%s|start=%u,duration=%u\n", chn+1,brate,inet_ntoa(ip),"Start",start_tm, time_duration);

	sdk_get_handle(0)->devobj[0].streamFilter[cmd.channel - 1].bit_his_r = 0x01;// 0x01 - video, 0x02 - audio
	cmd.cmd_args[10] = ULK_HIST_NORMAL;// video + audio
	cmd.cmd_args_len += 1;

	phist->his_info[chn].his_flag = flag; // pack flag: ts or frame
	phist->his_info[chn].his_enable=1; // for ack search result
	phist->his_ack[chn].channel_index = cmd.channel;
	phist->his_ack[chn].rate = brate;
	phist->his_ack[chn].ts_ts= start_tm;
	phist->his_ack[chn].ts_duration = time_duration;
	phist->his_ack[chn].ts_size = 0;// set it after 

	Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));	
    }
    if(mask & 0x0002)
    {
	DEBUGF("0x0002...\n");
	tmp2 = READ_4BYTE(pos);
	pos += 4;

	phist->his_info[chn].his_req_id_length = tmp2;				 
	memcpy(phist->his_info[chn].his_request_id, pos, tmp2);		
	pos += tmp2;
    }

    return (uint16)(pos - pos);
}


static int start_audio_talk_helper(char *user_name,uint32 seq,uint32 audio_total_len,uint32 from)
{
    Audio_Info_struct   ad_info ;

    ad_info.seq = seq;
    ad_info.type = ULK_AUDIO_TYPE_AAC;/* default */
    ad_info.audio_total_len = audio_total_len;
    ad_info.from = from;
    memset(ad_info.user_name, 0, sizeof(ad_info.user_name));
    strncpy(ad_info.user_name , user_name, sizeof(ad_info.user_name));

    Audio_msg_Add_queue((char*)&ad_info, sizeof(Audio_Info_struct));

    Audio_Talk_Buf_Struct *ptalk_buf = &sdk_get_handle(0)->talk.talk_buf;
    Audio_Talk_Ctrl_Struct *ptalk_ctrl = &sdk_get_handle(0)->talk.talk_ctrl;
    if (ptalk_ctrl->flag == 0)
    {
	ptalk_ctrl->flag = 1;
	memset((char *)ptalk_buf, 0, sizeof(Audio_Talk_Buf_Struct));
	sem_init(&ptalk_ctrl->play_ok, 0, 0);

	pool_add_worker(&deal_talk_thread, 0);
	DEBUGF("start deal_talk_thread task\n");
    }
    return 0;
}

int Unpack_MsgS2DUserDataNotify(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len < 8)
    {
	return	-1;
    }

    uint8 *pos = (uint8 *)buf;
    uint32  mask, flag, tmp2;
    char	*user_name;
    uint32	seq;
    uint32	audio_total_len;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;
    flag = flag ;   /* 防止警告 */

    DEBUGF("audio notify.\n");

    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");

	user_name = (char*)pos;

	//DEBUGF("username=%s\n", user_name);
	tmp2 = strlen((char*)pos) + 1;		
	pos += tmp2;

	seq = READ_4BYTE(pos);
	pos += 4;

	audio_total_len = READ_4BYTE(pos);
	pos += 4;

	start_audio_talk_helper(user_name,seq,audio_total_len,CMD_FROM_SERVER);
    }

    return (uint16)(pos - pos);
}

int Unpack_MsgS2DNVRHistoryListQuery(int skfd, char *buf, uint16 len)
{
    CMD_PARAM_STRUCT   cmd;
    if(buf == NULL || len < 8)
    {
	return	-1;
    }
    int arg_offset = 0;
    uint8 *pos = (uint8 *)buf;
    uint32	 mask, flag;
    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;
    flag = flag ;   /* 防止警告 */
    arg_offset = 0;
    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");
	uint8 chanl_index = *pos;
	cmd.channel=chanl_index;
	cmd.cmd_id=HISTORY_LIST;
	pos += 1;
	uint16 rate = READ_2BYTE(pos);
	pos += 2;
	cmd.cmd_args[arg_offset++] = rate & 0xFF;
	cmd.cmd_args[arg_offset++] = (rate >> 8) & 0xFF;
    }
    if(mask & 0x0002)
    {
	DEBUGF("0x0002...\n");
	uint32 starttime = READ_4BYTE(pos);
	pos += 4;
	uint32 endtime = READ_4BYTE(pos);
	pos += 4;
	cmd.cmd_args[arg_offset++] = starttime & 0xFF;
	cmd.cmd_args[arg_offset++] = (starttime >> 8) & 0xFF;
	cmd.cmd_args[arg_offset++] = (starttime >> 16) & 0xFF;
	cmd.cmd_args[arg_offset++] = (starttime >> 24) & 0xFF;
	cmd.cmd_args[arg_offset++] = endtime & 0xFF;
	cmd.cmd_args[arg_offset++] = (endtime >> 8) & 0xFF;
	cmd.cmd_args[arg_offset++] = (endtime >> 16) & 0xFF;
	cmd.cmd_args[arg_offset++] = (endtime >> 24) & 0xFF;
	printf ("start time = %d, end time = %d\n", starttime, endtime);
    }
    cmd.cmd_args_len=arg_offset;
    Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    return (uint16)(pos - pos);
}

int Pack_MsgD2CLoginResponse(char *buf,MsgD2CLoginResponse *msg)
{
    char 	*ptemp;
    uint16   templen;

    if(buf == NULL || msg== NULL)
    {
	return  -1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);

    if((msg->mask & 0x0001 ))
    {
	*ptemp++ =  msg->did.device_id_length;
	memcpy(ptemp, msg->did.device_id, msg->did.device_id_length);
	ptemp += msg->did.device_id_length;

	*ptemp++ =  (char)(msg->ret.error_code);
	*ptemp++ =  (char)(msg->ret.error_code >> 8);
	*ptemp++ =  (char)(msg->ret.error_code >> 16);
	*ptemp++ =  (char)(msg->ret.error_code >> 24);
	strcpy(ptemp,msg->ret.error_description);
	ptemp += strlen(msg->ret.error_description) + 1;// must end by '\0'
    }
    if(msg->mask&0x0002)
    {
	memcpy(ptemp, &msg->media_type, sizeof(msg->media_type));
	ptemp += sizeof(msg->media_type);

	memcpy(ptemp, &msg->dev_type, sizeof(msg->dev_type));
	ptemp += sizeof(msg->dev_type);

	//memcpy(ptemp, &msg->channel_num, sizeof(msg->channel_num));
	//ptemp += sizeof(msg->channel_num);
    }
    if(msg->mask&0x0004)
    {
	memcpy(ptemp, &msg->uchannelcount, sizeof(msg->uchannelcount));
	ptemp += sizeof(msg->uchannelcount);

	memcpy(ptemp, &msg->uSamplerate, sizeof(msg->uSamplerate));
	ptemp += sizeof(msg->uSamplerate);
	memcpy(ptemp, &msg->ubitLength, sizeof(msg->ubitLength));
	ptemp += sizeof(msg->ubitLength);
    }
    if(msg->mask&0x0008)
    {
	memcpy(ptemp, &msg->status_mask, sizeof(msg->status_mask));
	ptemp += sizeof(msg->status_mask);
    }

    templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_D2CLoginResponse);

    return templen;
}

int Unpack_MsgC2DLoginRequest(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len < 8)
    {
	return	-1;
    }

    uint8 *pos = (uint8 *)buf;
    uint32  mask, flag;
    device_id_t did = {0};

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    DEBUGF("[c2d login]mask=0x%x,flag=0x%x\n", mask, flag);
    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");
	did.device_id_length = *pos++;
	if(did.device_id_length < 21) memcpy(did.device_id,pos,did.device_id_length);
	pos += did.device_id_length;
	DEBUGF("len:%u,did:%s\n",did.device_id_length,did.device_id);
    }
    if(mask & 0x0002)
    {
	DEBUGF("0x0002...\n");
    }
    if(mask & 0x0004)
    {
	DEBUGF("0x0004...user data\n");
    }
    if(mask & 0x0008)
    {}

    ay_response_client_login(skfd,did);
    return (uint16)(pos - pos);
}

int Unpack_MsgC2DStatusReportRes(int skfd, char *buf, uint16 len)
{
    if(buf == NULL || len < 8)
    {
	return	-1;
    }

    uint8 *pos = (uint8 *)buf;
    uint32  mask, flag;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    flag = flag;//avoid warning

    DEBUGF("[c2d report res]mask=0x%x,flag=0x%x\n", mask, flag);
    if(mask & 0x0001)
    {
	DEBUGF("0x0001...\n");
	int cycle = READ_4BYTE(pos);
	cycle = cycle;
	pos += 4;
    }
    if(mask & 0x0002)
    {
	DEBUGF("0x0002...\n");
	int32 error_code_t = READ_4BYTE(pos);
	pos += 4;
	ERRORF("errcode=%d\n",error_code_t);
	pos += strlen((const char *)pos);
    }
    if(mask & 0x0004)
    {
	DEBUGF("0x0004...user data\n");
	CMD_PARAM_STRUCT   cmd;
	uint16 oem_data_len_t = READ_2BYTE(pos);
	pos += 2;

	uint8 *oem_data_t = pos;
	pos += oem_data_len_t;

	cmd.cmd_id = USER_DATA;
	cmd.cmd_args_len = oem_data_len_t;
	memcpy(cmd.cmd_args, oem_data_t, sizeof(cmd.cmd_args));
	Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));		
    }
    if(mask & 0x0008)
    {}

    return (uint16)(pos - pos);
}

int Unpack_MsgC2DActionNotify(int skfd, char *buf, uint16 len)
{
    if (buf == NULL || len < 8)
    {
        return	-1;
    }
    char buf_sys[256];
    uint8 *pos = (uint8 *)buf;
    uint32 mask, flag, tmp;
    uint8 chno, enable_upload;
    uint16 upload_rate;
    uint32 act;
    uint8 act_chn;

    mask = READ_4BYTE(pos);
    pos += 4;
    flag = READ_4BYTE(pos);
    pos += 4;

    DEBUGF("[c2d action]mask=0x%x,flag=0x%x\n", mask, flag);

    if (mask & 0x0001)
    {
        DEBUGF("0x0001...\n");
        enable_upload = *pos;
        DEBUGF("Unpack_MsgC2DActionNotify enable_upload = %d\n", enable_upload);
        upload_rate = READ_2BYTE(pos + 1);
        chno = *(pos + 3);

        if (flag & enm_control_record)
        {
            add_channel_cmd(HISTORY_STOP, chno, enable_upload, upload_rate, CMD_FROM_CLIENT);
        }
        else
        {// live stream control
            if (flag & enm_enable_audio)
            {
                add_channel_cmd(AUDIO_CTRL, chno, enable_upload, upload_rate, CMD_FROM_CLIENT);
            }

            if (flag & enm_enable_video)
            {
                DEBUGF("server don't stop motion detect upload\n");
                /*if (sdk_get_handle(0)->devinfo.vip_flag & C3VP_MDCLOUD_STORAGE && !enable_upload)
                {
                    if (sdk_get_handle(0)->inst_flag & SDK_FLAG_UPLOAD_MD_STREAM)
                    {
                        sdk_get_handle(0)->inst_flag &= ~SDK_FLAG_UPLOAD_MD_STREAM;
                    }
                }*/
                add_channel_cmd(VIDEO_CTRL, chno, enable_upload, upload_rate, CMD_FROM_CLIENT);
            }
        }
        pos += 4;
    }
    if (mask & 0x0002)
    {
        DEBUGF("msgAct ");
        act = READ_4BYTE(pos);
        pos += 4;
        act_chn = *pos;
        if (act == enm_server_reboot_device)
        {
            DEBUGF("reboot \n");
            //printf("-------------[todo] reboot in sdk -----------!\n");
            //system("reboot");
            PTZ_Channel_ctrl(act_chn, enm_server_reboot_device, 0);
        }
        else if (act == enm_server_reset_device)
        {
            DEBUGF("rm csp/* -rf\n");
            memset(buf_sys, 0, sizeof(buf_sys));
            snprintf(buf_sys, sizeof(buf_sys), "rm %s/csp/* -rf; reboot", sdk_get_handle(0)->devinfo.rw_path);
            //printf("------------- [todo]reset exec: %s  -----------!\n",buf_sys);
            //system(buf_sys);
            PTZ_Channel_ctrl(act_chn, enm_server_reset_device, 0);
        }
        else if (act == enm_server_transfer_audio)
        {
            add_channel_cmd(AUDIO_CTRL, act_chn, 1, upload_rate, CMD_FROM_CLIENT);
        }
        else if (act == enm_server_stop_transfer_audio)
        {
            add_channel_cmd(AUDIO_CTRL, act_chn, 0, upload_rate, CMD_FROM_CLIENT);
        }
        else 
        {
            DEBUGF("ptz,chn = %d,cmd = %d\n", act_chn, act);
            if (!(mask & 0x0010)) PTZ_Channel_ctrl(act_chn, act, 1); // default
        }
        pos += 1;
    }
    if ( mask & 0x0004 )
    {
        uint8 *from_user, *audio_data;
        uint32 seq;
        uint32 audio_total_len, audio_offset, audio_len;

        DEBUGF("0x0004...audio talk\n");

        from_user = pos;
        tmp = strlen((char*)from_user) + 1;
        pos += tmp;
        seq = READ_4BYTE(pos);
        pos += 4; /* audio seq */
        audio_total_len = READ_4BYTE(pos);
        pos += 4;
        audio_offset = READ_4BYTE(pos);
        pos += 4;
        audio_len = READ_4BYTE(pos);
        pos += 4;

        if (audio_offset == 0)
        {
            DEBUGF("start audio talk,seq = %d\n", seq);
            start_audio_talk_helper((char*)from_user, seq, audio_total_len, CMD_FROM_CLIENT);
        }

        audio_data = pos;
        pos += audio_len;

        DEBUGF("from_user = %s,total len = %d,offset = %d,len = %d\n", from_user, audio_total_len, audio_offset, audio_len);
        copy_talk_data(audio_total_len, audio_offset, audio_len, audio_data);
    }
    if (mask & 0x0008)
    {
        //uint16 expected_screenshot_cycle; /*期望的定时截图周期，以秒为单位，默认是10分钟 */
        pos += 2;
    }
    if (mask & 0x0010)
    {
        uint32 steps = READ_4BYTE(pos);
        DEBUGF("0x0010...ptz steps = %d\n", steps);
        PTZ_Channel_ctrl(act_chn, act, steps);
        pos += 4;
    }
    if (mask & 0x0020)
    {
        CMD_PARAM_STRUCT   cmd;
        cmd.channel =  1;
        cmd.cmd_id  = SD_CARD_CTRL;
        cmd.cmd_args[0] = *pos;
        cmd.cmd_args_len = 1;
        pos += 1;
        DEBUGF("0x0020...sd card ctrl,cmd = %d\n", cmd.cmd_args[0]);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }
    if (mask & 0x0040)
    {
        CMD_PARAM_STRUCT   cmd;
        cmd.channel =  1;
        cmd.cmd_id  = RECORD_CTRL;
        cmd.cmd_args[0] = *pos++;
        cmd.cmd_args[1] = *pos++;
        cmd.cmd_args_len = 2;
        DEBUGF("0x0040...record ctrl,cmd = %d,mode = %d\n",cmd.cmd_args[0],cmd.cmd_args[1]);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }
    if (mask & 0x0080)
    {
        CMD_PARAM_STRUCT cmd;
        uint16 prepos = 0;
        act = *pos++;
        switch (act)
        {
            case enm_pp_noop: cmd.cmd_id = PTZ_SET_PRE_LOCAL; break;
            case enm_pp_set: cmd.cmd_id = PTZ_SET_PRE_LOCAL; break;
            case enm_pp_clear: cmd.cmd_id = PTZ_CLEAR_PRESET; break;
            case enm_pp_jump: cmd.cmd_id = PTZ_CALL_PRE_LOCAL; break;
        }
        cmd.channel = *pos++;
        prepos = READ_2BYTE(pos);
        cmd.cmd_args[0] = *pos++;
        cmd.cmd_args[1] = *pos++;
        cmd.cmd_args_len = 2;
        DEBUGF("0x0080...preset ctrl,cmd = %d,chn = %d,pos = %hu\n", cmd.cmd_id, cmd.channel, prepos);
        Msg_Cmd_Add_down_queue((char*)&cmd, sizeof(CMD_PARAM_STRUCT));
    }

    return (uint16)(pos - pos);
}

int Unpack_Msg(int skfd, char *pbuf, int len)
{
    MSG_HEADER_C3 	*temphd;
    int  		rslt;

    if(len < sizeof(MSG_HEADER_C3))
    {
	ERRORF("msglen is too short:%d\n", len);
	return -1;
    }
    temphd = (MSG_HEADER_C3*)pbuf;

    DEBUGF("msg.len=%d,%d\n", len, temphd->size);	
    if ( len < temphd->size )
    {		
	return -1;
    }
    if ( temphd->size > FRAME_RECV_SIZE + 512 )
    {
	return -2;
    }
    if(temphd->msg_type == CID_DeviceRegisterResponse || CID_StatusReportRes==temphd->msg_type)
	DEBUGF("size=%d, msgtype=0x%x\n", temphd->size, temphd->msg_type);
    else
	TRACEF("size=%d, msgtype=0x%x\n", temphd->size, temphd->msg_type);

    switch(temphd->msg_type)
    {
	case CID_DeviceRegisterResponse:    /* 注册响应 */
	    rslt = Unpack_MsgDeviceRegisterRespons(pbuf + 6, len - 6);
	    break;
	case CID_DeviceLoginResponse:       /* 登录响应 */
	    rslt = Unpack_MsgDeviceLoginResponse(pbuf + 6, len - 6);
	    break;
	case CID_StreamFrameDataRequest:    /* 帧请求 */
	    rslt = Unpack_MsgStreamFrameDataRequest(skfd, pbuf + 6, len - 6);
	    break;
	case CID_StatusReportRes:           /* 流服务器到设备返回 */
	    rslt = Unpack_MsgStatusReportRes(skfd, pbuf + 6, len - 6);
	    break;
	case CID_DeviceActionNofity:
	    rslt = Unpack_MsgDeviceActionNofity(skfd, pbuf + 6, len - 6);
	    break;
	case CID_S2DTSDataQuery:            /* 历史视频请求 */
	    rslt = Unpack_MsgS2DTSDataQuery(skfd, pbuf + 6, len - 6);
	    break;
	case CID_S2DUserDataNotify:         /* 对讲通知 */
	    rslt = Unpack_MsgS2DUserDataNotify(skfd, pbuf + 6, len - 6);
	    break;
	case CID_S2DNVRHistoryListQuery:
	    rslt = Unpack_MsgS2DNVRHistoryListQuery(skfd, pbuf + 6, len - 6);
	    break;

	case CID_C2DLoginRequest:
	    rslt = Unpack_MsgC2DLoginRequest(skfd, pbuf + 6, len - 6);
	    break;
	case CID_C2DActionNotify:
	    rslt = Unpack_MsgC2DActionNotify(skfd, pbuf + 6, len - 6);
	    break;
	case CID_C2DStatusReportRes:
	    rslt = Unpack_MsgC2DStatusReportRes(skfd, pbuf + 6, len - 6);
	    break;

	default:
	    rslt = AY_ERROR_MSGTYPE;
	    break;
    }
    return (rslt<0)?rslt:temphd->size;
}

//--------------------------------------------------------------------------------------------------------
int Pack_MsgDeviceAbilityReport(char *buf, MsgDeviceAbilityReport *msg)
{
	char *ptemp;
    uint16 templen;
    if(buf == NULL || msg== NULL)
    {
	return	-1;
    }

    ptemp = buf + 6;
    ptemp += fill_mask_and_flag(ptemp,msg->mask,msg->flag);
	if(msg->mask & 0x01)	
	{
		*ptemp++ = msg->max_stream_num_per_chn;
	}
	templen = (uint16)(ptemp - buf);
    fill_length_and_cmd(buf,templen,CID_DeviceAbilityReport);

    return templen;
}
