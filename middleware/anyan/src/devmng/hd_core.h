#ifndef __HD_CORE_H__
#define __HD_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define HD_BLSERVER_NAME "182.254.156.98" // test server
#define HD_BLSERVER_NAME "entry.device.serv.ulucu.com"
#define HD_BLSERVER_PORT 29023 

#define HD_PROTOCOL_VER	"2.1.2"

#define HD_HEART_INTERVAL 20 // seconds

#ifdef WIN32
#define MSG_NOSIGNAL 0
#endif

enum hd_status_machine
{
    HD_MACH_LOGIN_LDB = 0,
    HD_MACH_LDB_RECV,
    HD_MACH_LDB_ACK,
    HD_MACH_LOGIN_MGR,
    HD_MACH_MGR_RECV,
    HD_MACH_LOGIN_MGR_ACK,
    HD_MACH_MGR_CMD,
    HD_MACH_MGR_BEAT,
};

struct hd_cmd_dict
{
    char name[64];
    char type;/* 0 - int, 1 - string,3 - array json */
    int value;
    char string[256];
    struct hd_cmd_dict *parray;
    int array_len;
};

struct hd_dev_instance
{
    char blserver_name[32];
    unsigned short blserver_port;
    char devmgr_server_name[32];
    unsigned short devmgr_server_port;
    char token[256];
    int bl_sock;
    int mgr_sock;
    struct sockaddr mgr_addr;
    char devid[64];
    char last_serial[64];
    int last_errcode;
    char last_alarm_type[64];
    struct timespec last_alarm_ts;
    int login_mgr_ok;
};

typedef struct hd_dev_instance* HD_DEV_HANDLE;

char *hd_build_req(const char *cmd,char *serial,struct hd_cmd_dict values[],int num);
char *hd_build_ack(const char *cmd,char *serial,int errcode,const char *errstr, struct hd_cmd_dict values[],int num);
char *hd_build_ack_just_ok(const char *cmd,char *serial);

void *hd_process_thread(void *args);

#ifdef __cplusplus
}
#endif

#endif

