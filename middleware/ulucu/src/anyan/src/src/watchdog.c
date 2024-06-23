#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32

int share_mem_init(void)
{
    return 0;
}
int share_mem_uint(void)
{
    return 0;
}
void ulumon_msg_check(int msg_type)
{
    return ;
}

#else
#include <unistd.h>
#include <sys/shm.h>

#include "watchdog.h"
#include "define.h"
#include "ayutil.h"

struct shared_use_st
{
    int  len;
    char text[10];
};

static int shmid = -1;
void *shared_memory=(void *)0;
struct shared_use_st *shared_stuff = NULL;

int share_mem_init(void)
{
    shmid = shmget((key_t)1234,sizeof(struct shared_use_st),0666|IPC_CREAT);
    if(shmid == -1)
    {
        DEBUGF( "shmget failed\n");
        return -1;
    }

    shared_memory = shmat(shmid,(void *)0,0);
    if(shared_memory == (void *)-1)
    {
        DEBUGF("shmat failed\n");
        return -1;
    }
    DEBUGF("Memory attached at %X\n", (int)shared_memory);

    shared_stuff = (struct shared_use_st *)shared_memory;
    return 0;
}

int share_mem_uint(void)
{
    int ret = 0;
    if(shared_memory!=NULL)
    {
	shmdt(shared_memory);
	shared_stuff = shared_memory = NULL;
    }
    if(shmid != -1)
    {
	ret=shmctl( shmid, IPC_RMID, 0 );
	if( ret==0 )
	{
	    DEBUGF( "Shr mem rm ok\n");
	    shmid = -1;
	}
	else
	{
	    DEBUGF( "Shr mem rm fail,ret = %d,err:%s\n",ret,strerror(errno));
	    return -1;
	}
    }
    return 0;
}


void ulumon_msg_check(int msg_type)
{
    if ( shared_stuff != NULL && shmid != -1)
    {
	//DEBUGF("wthdog\n");
	shared_stuff->len = msg_type;
    }
    else if(shmid != -1)
    {
	share_mem_init();
    }
    else if(shared_stuff != NULL)
    {
	shared_memory = shmat(shmid,(void *)0,0);
	if(shared_memory == (void *)-1)
	{
	    //DEBUGF("shmat failed\n");
	    return ;
	}
	DEBUGF("Memory attached at %X\n", (int)shared_memory);

	shared_stuff = (struct shared_use_st *)shared_memory;
    }
}

/*  APIs should be Called in the watchdog process */

int  Ulu_SDK_Watchdog_init(void)
{
    return share_mem_init();
}

static   struct timespec check_period = {0,0};
int Ulu_SDK_Get_ulu_net_status(void)
{
    struct timespec now;
    if(shared_stuff == NULL)
	return -1;

    clock_gettime(CLOCK_MONOTONIC,&now);
    if(ayutil_cost_mseconds(check_period,now) > 15*1000)
    {/* min period is 15 seconds */
        check_period = now;
        if (shared_stuff->len > 0)
        {
            shared_stuff->len = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 1;
    }
}

#endif // __Linux__

