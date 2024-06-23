#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "Anyan_Device_SDK.h"

struct net_statistic_info
{
    uint32 bytes_tosend[5]; // total, 10s, 5min, 10min, 30min
    uint32 bytes_sendok[5];
    uint32 bytes_recvok[5];
    float  want_send_speed[5]; // kB/s
    float  real_send_speed[5]; // KB/s
    struct timespec last_ts[5];
};

extern struct net_statistic_info net_stat_info;

int trace_init_net_stat_info(void);
int trace_get_net_stat_info(struct net_statistic_info *netinfo);
int trace_update_net_stat_info(uint32 tosndbytes,uint32 sndokbytes, uint32 rcvbytes);

int Ulu_SDK_Refresh_Device_Info(const char *filename);

char *aymonitor_get_json_log(int format,const char *event,const char *msg);
int aymonitor_get_uplog(time_t htime,const char *event,const char *msg, char *ptext,int size);

#endif

