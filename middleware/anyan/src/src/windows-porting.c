#ifdef WIN32

#include "windows-porting.h"
#include "define.h"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

int clock_gettime(clockid_t clk_id,struct timespec *tp)
{
    if(tp==NULL) return -1;

    if(clk_id == CLOCK_MONOTONIC)
    {
	LARGE_INTEGER StartingTime;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency); 
	QueryPerformanceCounter(&StartingTime);

	tp->tv_sec = StartingTime.QuadPart/Frequency.QuadPart;
	tp->tv_nsec = ((StartingTime.QuadPart*1000000)/Frequency.QuadPart - tp->tv_sec*1000000)*1000;
    }
    else
    {
	FILETIME wintime; 
	ULARGE_INTEGER wint;
	GetSystemTimeAsFileTime((FILETIME*)&wintime); // 100 nano seconds
	wint.LowPart = wintime.dwLowDateTime;
	wint.HighPart = wintime.dwHighDateTime;
	wint.QuadPart -= 116444736000000000;  //1jan1601 to 1jan1970
	tp->tv_sec  = wint.QuadPart / 10000000;//seconds
	tp->tv_nsec = (wint.QuadPart % 10000000)*100;//nano-seconds
    }
    return 0;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if(tv==NULL) return -1;

    struct timespec wintime;
    clock_gettime(CLOCK_REALTIME,&wintime);
    
    tv->tv_sec = wintime.tv_sec;
    tv->tv_usec = wintime.tv_nsec/1000;
    return 0;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	if (timep == NULL || result == NULL)
		return NULL;

	FILETIME ftime;
	SYSTEMTIME stime;
	ftime.dwHighDateTime = *timep;
	ftime.dwLowDateTime = 0;

	FileTimeToSystemTime(&ftime,&stime);

	result->tm_isdst = 0;
	result->tm_year = stime.wYear-1900;
	result->tm_mon = stime.wMonth-1;
	result->tm_mday = stime.wDay;
	result->tm_hour = stime.wHour;
	result->tm_min = stime.wMinute;
	result->tm_sec = stime.wSecond;
	result->tm_wday = stime.wDayOfWeek;

	return result;
}

unsigned int sleep(unsigned int seconds)
{
	Sleep(seconds * 1000);
	return 0;
}

unsigned int usleep(unsigned int us)
{
	unsigned int ms = us / 1000;
	if (ms == 0) ms = 1;
	Sleep(ms);
	return 0;
}

int network_init() {
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	return WSAStartup(wVersionRequested, &wsaData);
}
void network_deinit() {
	WSACleanup();
}

#endif

