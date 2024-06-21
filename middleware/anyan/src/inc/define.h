#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "typedefine.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_COMPANY_CAMERA_COUNT	500

#define FRIEND_COMPANY_MAN_RIGHT_MASK	1 //友商管理
#define EMPLOYEE_MAN_RIGHT_MASK		2 //员工管理
#define VISIT_HISTORY_MAN_MASK		4 //访问记录管理
#define CAMERA_MAN_RIGHT_MASK		8 //camera管理
#define GROUP_MAN_MASK			16//群管理
#define CHAT_CONTENT_MAN_MASK		32//聊天内容管理
#define VIDEO_LOOK_MASK			64//观看视频

#define    READ_4BYTE(p)  ((uint32)(*(p))+((uint32)(*(p+1))<<8)+((uint32)(*(p + 2))<<16)+((uint32)(*(p + 3))<< 24))
#define    READ_2BYTE(p)  ((uint16)(*(p))+((uint16)(*(p+1))<<8))

void ay_debug_printf(int level, const char* file, int line, const char *pszFormat, ...);
int ay_printf(int level, const char *format, ...);
void aydebug_printf(char *funNameStr, const char *buf, unsigned short buflen);
void aydebug_vindicate_logfile(void);
int aydebug_get_log(char log[],int size);

void set_thread_name(const char *desc);

typedef enum
{
    AYE_LOG_LEVEL_INFO = 0,
    AYE_LOG_LEVEL_DEBUG,
    AYE_LOG_LEVEL_TRACE,
    AYE_LOG_LEVEL_ERROR,
}AYE_LOG_LEVEL;

#ifdef WIN32

#define DEBUGF	printf
#define TRACEF	printf
#define ERRORF	printf

#else

#define DEBUG_EN  //only for test

#ifdef DEBUG_EN
#define DEBUG(format, ...) 	ay_debug_printf(AYE_LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define TRACE(format, ...) 	ay_debug_printf(AYE_LOG_LEVEL_TRACE, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ERROR(format, ...) 	ay_debug_printf(AYE_LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)	ay_printf(AYE_LOG_LEVEL_DEBUG,format, ##__VA_ARGS__)
#define TRACE(format, ...)	ay_printf(AYE_LOG_LEVEL_TRACE,format, ##__VA_ARGS__)
#define ERROR(format, ...) 	ay_printf(AYE_LOG_LEVEL_ERROR,format, ##__VA_ARGS__)
#endif

#endif

#ifdef __cplusplus
};
#endif

#endif//__DEFINE_H__
