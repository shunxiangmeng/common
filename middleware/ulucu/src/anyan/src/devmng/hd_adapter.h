#ifndef __HD_ADAPTER_H__
#define __HD_ADAPTER_H__

#include "cJSON.h"
#include "hd_core.h"
#include "AnyanDeviceMng.h"

void ADM_Set_CmdCallback(struct dm_cmd_callback cbfuncs);

int hd_res_operation_unsupport(HD_DEV_HANDLE hdh, const char *ack_cmd);
int hd_req_get_device_version(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_start_update(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_open_debug(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_close_debug(HD_DEV_HANDLE hdh, ayJSON *pdata);

int hd_req_set_motion_detect(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_get_motion_detect(HD_DEV_HANDLE hdh, ayJSON *pdata);

int hd_req_start_screenshot(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_query_record(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_start_download(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_stop_download(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_direction_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_stop_ptzctrl(HD_DEV_HANDLE hdh, ayJSON *pdata);

int hd_req_lens_init(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_focus_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_zoom_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_aperture_ctrl(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_set_preset_point(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_del_preset_point(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_set_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_del_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_start_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata);
int hd_req_stop_cruise(HD_DEV_HANDLE hdh, ayJSON *pdata);

/* 
 * report motion detect alarm event to HD Mgr server
 */
int hd_notify_alarm_event(HD_DEV_HANDLE hdh,struct dm_notify_alarm_msg *msg);

int hd_notify_download_start(HD_DEV_HANDLE hdh,int sock, const char *token);
int hd_notify_download_keepalive(HD_DEV_HANDLE hdh,int sock);
int hd_notify_download_finish(HD_DEV_HANDLE hdh,int sock);


#endif

