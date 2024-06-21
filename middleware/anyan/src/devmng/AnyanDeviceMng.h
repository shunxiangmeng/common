#ifndef __ANYAN_DEVICE_MNG_H__
#define __ANYAN_DEVICE_MNG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 设备管理接口定义错误码
 *
 * @note 除去这些协议已经定义的错误码外,应用也可以自定义其它未明确包含的错误码,同时添加错误说明信息
 * @sa struct dm_ack_res
 */
enum E_DEV_ERROR_CODE
{
    E_DEV_ERROR_OK = 0,
    E_DEV_ERROR_OPERATION_NOT_SUPPORT = -1000,
    E_DEV_ERROR_PARA_INVALID,
    E_DEV_ERROR_INTERNAL,
    E_DEV_ERROR_FILE_NOT_EXIST,
    E_DEV_ERROR_PROTOCOL_MISTAKE,
    E_DEV_ERROR_PROTOCOL_VERSION_MISMATCH,
    E_DEV_ERROR_REALPLAY_ALREADY,
    E_DEV_ERROR_REALPLAY_FAILED,
    E_DEV_ERROR_DOWNLOAD_ALREADY,
    E_DEV_ERROR_DOWNLOAD_FAILED,
    E_DEV_ERROR_PLAYBACK_ALREADY,
    E_DEV_ERROR_PLAYBACK_FAILED,
    E_DEV_ERROR_CAPTURE_FAILED,
};

struct dm_ack_res
{
    int error; // see @enum E_DEV_ERROR_CODE
    char reason[256];
};

struct dm_get_device_version_req
{
    int data;
};

struct dm_get_device_version_ack
{
    struct dm_ack_res ack;
    char version[64];
    char devsn[64];
    char ulusn[64];
};

struct dm_start_update_req
{
    char url[256];
    struct check_data
    {
        char type[64];
        char value[64];
        unsigned size;
    } check;
};

struct dm_start_update_ack
{
    struct dm_ack_res ack;
};

struct dm_start_screenshot_req
{
    int channel;
};

struct dm_start_screenshot_ack
{
    struct dm_ack_res ack;
    char image_name[64];    /* 设备sn_通道号_时间戳_随机数.文件扩展名,随机数解决同时间戳收到多个指令 */
    unsigned int image_size;
    char cloud_storage_path[64];
    unsigned short cloud_storage_type; // 1 - upai cloud
    unsigned short channel;
    unsigned int excute_time;
    unsigned int upload_start_time;
    unsigned int upload_end_time;
};

struct dm_notify_alarm_msg
{
    struct dm_ack_res ack;
    char type[32];          /* motion_detect_alarm移动侦测，region_boundary_alarm越界侦测,region_intrude_alarm闯入侦测 */
    time_t time;
    int channel;
    char image_name[64];                /* 设备sn_通道号_时间戳_随机数.文件扩展名,随机数解决同时间戳收到多个指令;如果没图像,则清零 */
    unsigned int image_size;            /* 如果没图像,应该置0 */
    char cloud_storage_path[64];        /* 图像在第三方云平台上的存储访问路径,比如又拍云/七牛云/阿里云等 */
    unsigned short cloud_storage_type;  /* 1 - upai cloud,当前仅支持又拍云 */
    unsigned int upload_start_time;
    unsigned int upload_end_time;
};

struct dm_query_record_req
{
    int channel;
    unsigned int s_tm;
    unsigned int e_tm;
};

struct dm_record_list {
    char record_name[64];
    unsigned int size;
    unsigned int s_tm;
    unsigned int e_tm;
};

struct dm_query_record_ack
{
    struct dm_ack_res ack;
    unsigned int free_need;// 1 - need to free list
    unsigned int list_num;
    struct dm_record_list *record_list;
};

struct dm_start_download_req
{
    char token[128];
    int channel;
    char record_name[64];
    unsigned int s_tm;
    unsigned int e_tm;
    unsigned int size;
    int audio_frame_index;      /* 必填 -1表示不下载该流,>= 0表示下载流的帧偏移 */
    int video_frame_index;      /* 必填 -1表示不下载该流,>= 0表示下载流的帧偏移 */
    char server_ip[32];
    unsigned short server_port;
    char transport_protocol[16];// "udp","tcp"
};

struct dm_start_download_ack
{
    struct dm_ack_res ack;
};

struct dm_stop_download_req
{
    int channel;
    char record_name[64];
    unsigned int s_tm;
    unsigned int e_tm;
    unsigned int size;
};

struct dm_stop_download_ack
{
    struct dm_ack_res ack;
};

struct dm_direction_ctrl_req
{
    int channel;
    int speed;
    char direction[32];
};

struct dm_direction_ctrl_ack
{
    struct dm_ack_res ack;
};

struct dm_stop_ctrl_req
{
    int channel;
};

struct dm_stop_ctrl_ack
{
    struct dm_ack_res ack;
};

struct dm_lens_init_req
{
    int dummy;
};

struct dm_lens_init_ack
{
    struct dm_ack_res ack;
};

struct dm_focus_ctrl_req
{
    int channel;
    int focus;  /* 1 - 大, 0 - 小 */
};

struct dm_focus_ctrl_ack
{
    struct dm_ack_res ack;
};

struct dm_zoom_ctrl_req
{
    int channel;
    int zoom; /* zoom, 1大，0小 */
    int speed;
};

struct dm_zoom_ctrl_ack
{
    struct dm_ack_res ack;
};

struct dm_aperture_ctrl_req
{
    int channel;
    int aperture;   /* zoom, 1大，0小 */
    int speed;
};

struct dm_aperture_ctrl_ack
{
    struct dm_ack_res ack;
};

struct dm_set_preset_point_req
{
    int channel;
    int number; // start from 0
};

struct dm_set_preset_point_ack
{
    struct dm_ack_res ack;
};

struct dm_del_preset_point_req
{
    int channel;
    int number; // start from 0
};

struct dm_del_preset_point_ack
{
    struct dm_ack_res ack;
};

struct dm_set_cruise_req
{
    int channel;
    char cruise_name[32];
    int preset_point_list[64];
};

struct dm_set_cruise_ack
{
    struct dm_ack_res ack;
};

struct dm_del_cruise_req
{
    int channel;
    char cruise_name[32];
};

struct dm_del_cruise_ack
{
    struct dm_ack_res ack;
};

struct dm_start_cruise_req 
{
    int channel;
    char cruise_name[32];
};

struct dm_start_cruise_ack
{
    struct dm_ack_res ack;
};

struct dm_stop_cruise_req 
{
    int channel;
};

struct dm_stop_cruise_ack
{
    struct dm_ack_res ack;
};

struct dm_period_list
{
    int week; // 0 - invalid, 1~7 - valid period
    char start[16]; // e.g."12:00:00"
    char end[16]; // eg."18:00:00"
};

struct dm_region_list
{
    int left;
    int top;
    int right;
    int bottom;
};

struct dm_set_motion_detect_req
{
    int channel;
    int sub_channel;
    int enable;
    int sensitivity;
    int snapshot_enable;
    int period_num;
    struct dm_period_list period_list[7];
    int region_num;
    struct dm_region_list region_list[16];
};

struct dm_set_motion_detect_ack
{
    struct dm_ack_res ack;
};

struct dm_get_motion_detect_req
{
    int channel;
    int sub_channel;
};

struct dm_get_motion_detect_ack
{
    struct dm_ack_res ack;
    int enable;
    int sensitivity;
    int snapshot_enable;
    int period_num;
    struct dm_period_list period_list[7];
    int region_num;
    struct dm_region_list region_list[16];
};

/*
 * 设备管理协议命令回调函数集定义
 *
 * @note 应用对支持的协议命令,实现对应的回调函数即可;如果不支持,只需要赋值为NULL
 * @note 建议在赋值之前,将函数集memset为0
 */
struct dm_cmd_callback
{
    int (*cmd_req_get_device_version)(struct dm_get_device_version_req *req,struct dm_get_device_version_ack *res);
    int (*cmd_req_start_update)(struct dm_start_update_req *req,struct dm_start_update_ack *res);
    int (*cmd_req_start_screenshot)(struct dm_start_screenshot_req *req,struct dm_start_screenshot_ack *res);
    int (*cmd_req_query_record)(struct dm_query_record_req *req,struct dm_query_record_ack *res);
    int (*cmd_req_start_download)(struct dm_start_download_req *req,struct dm_start_download_ack *res);
    int (*cmd_req_stop_download)(struct dm_stop_download_req *req, struct dm_stop_download_ack *res);

    /************ PTZ Feature ********************/
    int (*cmd_req_lens_init)(struct dm_lens_init_req *req,struct dm_lens_init_ack *res);
    int (*cmd_req_direction_ctrl)(struct dm_direction_ctrl_req *req,struct dm_direction_ctrl_ack *res);
    int (*cmd_req_stop_ctrl)(struct dm_stop_ctrl_req *req,struct dm_stop_ctrl_ack *res);
    int (*cmd_req_focus_ctrl)(struct dm_focus_ctrl_req *req,struct dm_focus_ctrl_ack *res);
    int (*cmd_req_zoom_ctrl)(struct dm_zoom_ctrl_req *req,struct dm_zoom_ctrl_ack *res);
    int (*cmd_req_aperture_ctrl)(struct dm_aperture_ctrl_req *req,struct dm_aperture_ctrl_ack *res);
    int (*cmd_req_set_preset_point)(struct dm_set_preset_point_req *req,struct dm_set_preset_point_ack *res);
    int (*cmd_req_del_preset_point)(struct dm_del_preset_point_req *req,struct dm_del_preset_point_ack *res);
    int (*cmd_req_set_cruise)(struct dm_set_cruise_req *req,struct dm_set_cruise_ack *res);
    int (*cmd_req_del_cruise)(struct dm_del_cruise_req *req,struct dm_del_cruise_ack *res);
    int (*cmd_req_start_cruise)(struct dm_start_cruise_req *req,struct dm_start_cruise_ack *res);
    int (*cmd_req_stop_cruise)(struct dm_stop_cruise_req *req,struct dm_stop_cruise_ack *res);

    int (*cmd_req_set_motion_detect)(struct dm_set_motion_detect_req *req,struct dm_set_motion_detect_ack *res);
    int (*cmd_req_get_motion_detect)(struct dm_get_motion_detect_req *req,struct dm_get_motion_detect_ack *res);

    /*** More Device Manage Cmds ... */
};

/*
 * 设备管理协议环境初始化接口
 *
 * @sa struct dm_cmd_callback
 * @param[in] cbfuncs 设备支持的命令回调函数集合
 * @ret 0 - success, -1 - fail,内存分配失败或者重复初始化
 */
int ADM_Init_Env(struct dm_cmd_callback cbfuncs);

/*
 * 设备管理环境去初始化接口
 *
 * @ret none
 */
void ADM_Exit_Env(void);

/*
 * 通知管理平台开始进行录像下载
 *
 * @param[in] sock 已经与下载服务器建立连接的套接字
 * @param[in] token 管理服务器下发下载命令中的认证token
 * @ret 0 - 发送成功, -1 - 发送失败
 * @note 参考demo中的接口使用
 */
int ADM_notify_download_start(int sock, const char *token);

/*
 * 设备与下载服务器之间的心跳维持
 *
 * @param[in] sock 已经与下载服务器建立连接的套接字
 * @ret 0 - 发送成功, -1 - 发送失败
 * @note 参考demo中的接口使用
 */
int ADM_notify_download_keepalive(int sock);

/*
 * 设备通知下载服务器录像下载任务结束
 *
 * @param[in] sock 已经与下载服务器建立连接的套接字
 * @ret 0 - 发送成功, -1 - 发送失败
 * @note 参考demo中的接口使用
 */
int ADM_notify_download_finish(int sock);

/*
 * 设备上报报警事件到管理平台
 *
 * @param[in] msg 报警事件的信息
 * @ret 0 - 上报成功,-1 - 环境未初始化, -2 - 还未登录平台成功, -3 - 5秒内同类型报警仅上报一次,-4 - 发送失败 
 * @note 应用需要根据移动侦测的参数进行报警管理与上报控制,参考get_motion_detect_req和set_motion_detect_req.
 */
int ADM_notify_alarm_event(struct dm_notify_alarm_msg *msg);

/*
 * 辅助接口:下载指定URL地址的内容
 *
 * @param[in] purl 指定的下载地址
 * @param[out] pout_len 下载内容的长度
 * @ret 返回的下载内容地址,内存由接口动态分配,因此必须在使用完后使用free函数释放掉!!
 * @note 该接口是SDK提供的工具辅助函数,应用可以选择使用
 */
char *ADM_Get_Webcontent_Helper(const char *purl, int *pout_len);

#ifdef __cplusplus
}
#endif

#endif
