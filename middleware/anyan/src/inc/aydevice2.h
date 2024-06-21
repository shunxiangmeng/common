#ifndef __AYDEVICE2_H__
#define __AYDEVICE2_H__

#include "ayentry.h"
#include "ay_sdk.h"

#define  AYDEVICE2_ENABLE 0

#define AYDEVICE2_SERVER_SN "sn.device2.ulucu.com"
#define AYDEVICE2_SERVER_PUBLIC "public.device2.ulucu.com"
#define AYDEVICE2_SERVER_LOG	"device.syslog.ulucu.com"

#define AYDEVICE2_SERVER_DNSLIST    "nsroot.biz.ulucu.com"
#define AYDEVICE2_SERVER_DEVENTRY "entry.device.serv.ulucu.com"
#define AYDEVICE2_SERVER_PICYUN	"c2-hd-img.upy-img.static.ulucu.com"
#define AYDEVICE2_SERVER_UPYUN	"v0.api.upyun.com"

int aydevice2_get_20sn(Dev_SN_Info *param,char *pou_deviceid);
int aydevice2_login(const char *sn20,int min_rate,int max_rate);
int aydevice2_get_token(const char *sn20,entry_token *token);

int aydevice2_upload_log(const char *log);
void aydevice2_query_dnslit(int arg);
int aydevice2_nslookup_host(const char *host,st_ay_net_addr *ip);
int aydevice2_update_domainip(const char *domain,const char *file);

#endif

