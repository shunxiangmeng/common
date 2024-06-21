#ifndef __AY_ENTRY_H__ 
#define __AY_ENTRY_H__ 

#include "ghttp.h"
#include "cJSON.h"
#include "typedefine.h"
#include "Anyan_Device_SDK.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DEVICE_MAX_LEN 20
#define ENTRY_SERVER_MAX_NUM 50

#define AYENTRY_SERVER_PUBLIC "public.api.anyan.com" // default: 115.159.17.225
#define AYENTRY_SERVER_CONFIG "config.anyan.com"     // default: 115.159.111.107

typedef struct entry_server_addr_s 
{
    char IP[64];
    unsigned int port;
} entry_server_addr;

typedef struct entry_server_list_s
{
    int num;
    entry_server_addr list[ENTRY_SERVER_MAX_NUM];
} entry_server_list;

enum entry_statustype_re
{
    ENTRY_STATUS_NORMAL = 1,
    ENTRY_STATUS_BLACK_LIST = 2,
    ENTRY_STATUS_NOT_IN_DB = 4
};

enum user_type_e
{
    USER_TYPE_PERSONAL = 1, /* 个人用户 */
    USER_TYPE_COMPANY = 2,  /* 企业用户 */
};

/* 通道付费状态 */
enum channel_paidstatus_e
{
    CHANNEL_PAIDSTATUS_NUKNOW,   /* 未知 */
    CHANNEL_PAIDSTATUS_UP,       /* 已付 */
    CHANNEL_PAIDSTATUS_TEST,     /* 测试 */
};

typedef struct channel_paidinfo_s
{
    int channel_id;
    enum channel_paidstatus_e paidstatus;
} channel_paidinfo,*pchannel_paidinfo;

typedef struct entry_data_s
{
    int oem;
    int ichancel_count;
    channel_paidinfo pchn_paid_head[AY_MAX_CHANNEL_NUM];
    enum entry_statustype_re status;
    enum user_type_e iuser_type;
    char device_id[21];
    char token[384];
} entry_data, *pentry_data;

typedef struct entry_token_s
{
    int  icode;
    char szmessage[128];
    entry_data data;
} entry_token, *pentry_token;

typedef struct entry_request_param_s
{
    char device_id[24];
    version_t  ver;
    BOOL   audio;
    BOOL   recv_audio;
    BOOL   tilt_spin;      /* 云台上下旋转 */
    BOOL   spin;           /* 云台左右旋转 */
    BOOL   zoom;           /* 云台变焦 */
    BOOL   preset;         /* 预置位 */
    BOOL   disk;           /* 有硬盘 */
    uint32 rate_count;
    uint16 ratelist[2];
} entry_request_param, *pentry_request_param;

int ayentry_get_token(entry_token* ou_token, entry_request_param *param);
int ayentry_get_entryserver(entry_server_list* pserver_list);
int ayentry_register_device(char* pou_deviceid, Dev_SN_Info *param);
int ay_request_reset_device(const char *devid, const char *dev_token);
int ayentry_get_alarm_param(ayJSON *pjson,ULK_Alarm_Struct *palarm);
char *request_webserver_content(const char *purl, int *pout_len,int (*handle)(const char *in,int inlen,char out[],int outlen));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif 