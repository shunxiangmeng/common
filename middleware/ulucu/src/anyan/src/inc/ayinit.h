#ifndef __AY_INIT__H__
#define __AY_INIT__H__

#include "typedefine.h"
#include "http_base64.h"

#define    LOCAL_PORT_VLU    5000

#ifdef __cplusplus
extern "C"
{
#endif
    void*   Regist_thread_C3(void *args);
    void*   Interact_CallBack_Thread(void* args);
    void*   device_discovery_thread(void *args);
    void*   log_process_thread(void *args);

    uint32  get_dev_attr_to_flag(void);

#ifdef __cplusplus
};
#endif

#endif


