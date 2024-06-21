
#ifndef __PROTOCOL_QUERY_DEVICE_H__
#define __PROTOCOL_QUERY_DEVICE_H__

#include <string.h>
#include <stdio.h>

#include "protocol_exchangekey.h"
#include "cdh_crypt_lib.h"
#include "cdiffie_hell_man.h"
#include "protocol_device.h"

#pragma pack(1)
typedef struct MsgDeviceStatusQuery
{
    uint32	magic1;
    uint32  magic2;
} MsgDeviceStatusQuery;

typedef struct MsgDeviceStatusQueryResponse
{
    uint32	mask;

    //0x01
    uint32 ip;
    uint16 port;
    char factory[255+1];    /* 厂商 */
    char model[64+1];       /* 型号 */
    char device_type[8];    /* dvr,nvr,ipc */
    uint32 channel_num;
    uint32 use_type;

    //0x02
    char orig_id[64];
    uint32 factory_id;
    char factory_abbreviation[64];

    //0x04
    char device_id[21];

    //0x08
    uint16 ver[4];

    //0x10
    uint16 lan_listen_port;
    uint32 sdkrel;
} MsgDeviceStatusQueryResponse;
#pragma pack()

int Unpack_MsgDeviceStatusQuery(char *pMsgBuf);
int Pack_MsgDeviceStatusQueryResponse(char *buf, MsgDeviceStatusQueryResponse *pRsp);

#endif
