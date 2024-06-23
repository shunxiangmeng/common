#include "protocol_query_device.h"

int Unpack_MsgDeviceStatusQuery(char *pMsgBuf)
{
    MSG_HEADER_C3* pMsgHead = (MSG_HEADER_C3*)(pMsgBuf);
    MsgDeviceStatusQuery* pReq = (MsgDeviceStatusQuery*)(pMsgBuf+sizeof(MSG_HEADER_C3));

    if(pMsgHead->size!= 14 )
    {
        return -1;
    }

    if(pMsgHead->msg_type != 0x80010001)
    {
        return -1;
    }
    
    if( (pReq->magic1 != 14) || (pReq->magic2 != 14) )
    {
        return -1;
    }

    return 0;
}

int Pack_MsgDeviceStatusQueryResponse(char *buf, MsgDeviceStatusQueryResponse *pRsp)
{
    char *ptemp;
    uint16 templen;
    if(buf == NULL || pRsp== NULL)
    {
        return  -1;
    }
    
    ptemp = buf + 6;//预留6字节填充长度和类型
    
    *ptemp++ = (char)(pRsp->mask);
    *ptemp++ = (char)(pRsp->mask>>8);
    *ptemp++ = (char)(pRsp->mask>>16);
    *ptemp++ = (char)(pRsp->mask>>24);
    
    if(pRsp->mask & 0x0001)
    {
        *ptemp++ = (char)(pRsp->ip);
        *ptemp++ = (char)(pRsp->ip>>8);
        *ptemp++ = (char)(pRsp->ip>>16);
        *ptemp++ = (char)(pRsp->ip>>24);

        *ptemp++ = (char)(pRsp->port);
        *ptemp++ = (char)(pRsp->port>>8);

        templen = strlen(pRsp->factory);
        memcpy(ptemp, pRsp->factory, templen);
        ptemp += templen;
        *ptemp++ = '\0';

        templen = strlen(pRsp->model);
        memcpy(ptemp, pRsp->model, templen);
        ptemp += templen;
        *ptemp++ = '\0';

        templen = strlen(pRsp->device_type);
        memcpy(ptemp, pRsp->device_type, templen);
        ptemp += templen;
        *ptemp++ = '\0';

        *ptemp++ = (char)(pRsp->channel_num);
        *ptemp++ = (char)(pRsp->channel_num>>8);
        *ptemp++ = (char)(pRsp->channel_num>>16);
        *ptemp++ = (char)(pRsp->channel_num>>24);

        *ptemp++ = (char)(pRsp->use_type);
        *ptemp++ = (char)(pRsp->use_type>>8);
        *ptemp++ = (char)(pRsp->use_type>>16);
        *ptemp++ = (char)(pRsp->use_type>>24);
    }
    
    if(pRsp->mask & 0x0002)
    {
        templen = strlen(pRsp->orig_id);
        memcpy(ptemp, pRsp->orig_id, templen);
        ptemp += templen;
        *ptemp++ = '\0';

        *ptemp++ = (char)(pRsp->factory_id);
        *ptemp++ = (char)(pRsp->factory_id>>8);
        *ptemp++ = (char)(pRsp->factory_id>>16);
        *ptemp++ = (char)(pRsp->factory_id>>24);

        templen = strlen(pRsp->factory_abbreviation);
        memcpy(ptemp, pRsp->factory_abbreviation, templen);
        ptemp += templen;
        *ptemp++ = '\0';
    }
    
    if(pRsp->mask & 0x0004)
    {
        templen = strlen(pRsp->device_id);
        memcpy(ptemp, pRsp->device_id, templen);
        ptemp += templen;
        *ptemp++ = '\0';
    }

    if(pRsp->mask & 0x0008)
    {
        memcpy(ptemp, &pRsp->ver, sizeof(pRsp->ver));
        ptemp += sizeof(pRsp->ver);
    }
    if(pRsp->mask & 0x0010)
    {
	*ptemp++ = (pRsp->lan_listen_port)&0xff;
	*ptemp++ = (pRsp->lan_listen_port>>8)&0xff;

	*ptemp++ = (pRsp->sdkrel)&0xff;
	*ptemp++ = (pRsp->sdkrel>>8)&0xff;
	*ptemp++ = (pRsp->sdkrel>>16)&0xff;
	*ptemp++ = (pRsp->sdkrel>>24)&0xff;
    }

    templen = (uint16)(ptemp - buf);        //填充长度
    *buf=(char)(templen);
    *(buf + 1)=(char)(templen >> 8);
    
    *(buf + 2)=(char)CID_DeviceStatusQueryResponse;     //消息类型
    *(buf + 3)=(char)(CID_DeviceStatusQueryResponse >> 8);
    *(buf + 4)=(char)(CID_DeviceStatusQueryResponse >> 16);
    *(buf + 5)=(char)(CID_DeviceStatusQueryResponse >> 24);
    
    return *(uint16*)(buf);//返回长度	
}

