#ifndef __WINDOWS_PORTING_H__
#define __WINDOWS_PORTING_H__

#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>

#pragma warning(disable:4996)
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "Ws2_32.lib")

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#define strdup _strdup
#define strcasecmp lstrcmp

typedef int clockid_t;
typedef long suseconds_t;
typedef int socklen_t;

//struct timespec
//{
//   time_t tv_sec;
//   long tv_nsec;
//};

//struct timeval {
//    time_t      tv_sec;     /* seconds */
//    suseconds_t tv_usec;    /* microseconds */
//};

struct timezone {
    int tz_minuteswest;     /* minutes west of Greenwich */
    int tz_dsttime;         /* type of DST correction */
};

int clock_gettime(clockid_t clk_id,struct timespec *tp);
int gettimeofday(struct timeval *tv, struct timezone *tz);
struct tm *localtime_r(const time_t *timep, struct tm *result);
unsigned int usleep(unsigned int us);
unsigned int sleep(unsigned int seconds);
int network_init();
void network_deinit();
#endif

