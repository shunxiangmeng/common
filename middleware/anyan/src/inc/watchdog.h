#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#if __cplusplus
extern "C"{
#endif /* __cplusplus */

int share_mem_init(void);
int share_mem_uint(void);
void ulumon_msg_check(int msg_type);

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* _WATCHDOG_H_ */




