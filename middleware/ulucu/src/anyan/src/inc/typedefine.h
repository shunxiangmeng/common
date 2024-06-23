#ifndef __TYPEDEFINE_H__
#define __TYPEDEFINE_H__

#include "Anyan_Device_SDK.h"

#ifdef WIN32
    #include "pthreads-w/pthread.h"
    #include "pthreads-w/sched.h"
    #include "pthreads-w/semaphore.h"
    #include "windows-porting.h"
#else

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>  
#include <semaphore.h>

#define closesocket close
typedef unsigned int        DWORD;
typedef unsigned short      WORD;
#endif

#if 0
typedef char  		int8;
typedef short 		int16;
typedef int		int32;
typedef unsigned char 	uint8;
typedef unsigned short 	uint16;
typedef unsigned int  	uint32;
typedef unsigned long long  uint64;
#endif

typedef unsigned int	    UINT;
typedef int		    BOOL;
typedef unsigned char	    BYTE;
typedef BYTE*		    PBYTE;

typedef unsigned short	    version_t[4];

#ifndef NULL
#define NULL    		    ((void *)0)
#endif
typedef enum
{
	false = 0,
	true  = 1
}bool;
#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

typedef struct ay_net_addr
{
    char ipstr[32];
    unsigned short port;
}st_ay_net_addr;

#endif //__TYPEDEFINE_H__

