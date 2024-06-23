#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "define.h"
#include "monitor.h"
#include "ay_sdk.h"

static void aydebug_get_time(char buf[],int size)
{
    struct tm 	local;
    time_t 	t;

    t = time(NULL);
    localtime_r(&t,&local);
    snprintf(buf,size, "[%02d:%02d:%02d]", local.tm_hour ,local.tm_min, local.tm_sec);
}

static int aydebug_read_cache(char log[],int size)
{
    ay_sdk_handle psdk = sdk_get_handle(0);
    if(psdk==NULL)
	return -1;

    struct debug_log_cache *pcache = &psdk->debug.log_cache;

//    if(pcache->mutex==0)
//    {
//	pcache->mutex = 2;
//    }
//    else
//    {
//	return 0;
//    }
    pthread_mutex_lock(&pcache->mutex);
    if(pcache->pbuf && pcache->cap>0 && pcache->size>0)
    {
	if(size > 4096) size = 4096;
	int len = size-1;
	memset(log,0,size);
	if(pcache->roffset < pcache->woffset)
	{
	    len = (pcache->woffset - pcache->roffset > len)?len:(pcache->woffset - pcache->roffset);
	    memcpy(log,pcache->pbuf+pcache->roffset,len);
	    pcache->roffset += len;
	}
	else
	{
	    if(pcache->cap - pcache->roffset > len)
	    {
		memcpy(log,pcache->pbuf+pcache->roffset,len);
		pcache->roffset += len;
	    }
	    else
	    {
		len = pcache->cap - pcache->roffset;
		memcpy(log,pcache->pbuf+pcache->roffset,len);
		pcache->roffset = 0; 
	    }
	}

	pcache->size -= len;
	//printf("$$$$$$$ woff = %d,roff = %d,size = %d\n",pcache->woffset,pcache->roffset,pcache->size);
//	pcache->mutex = 0;
    pthread_mutex_unlock(&pcache->mutex);

	return len;
    }
//    pcache->mutex = 0;
    pthread_mutex_unlock(&pcache->mutex);

    return 0;
}

static void aydebug_write_cache(struct debug_log_cache *pcache,const char *log)
{
    int len = strlen(log);
    int loff = 0,left = 0;

    //printf("@@@@@(%d)%s",strlen(log),log);

//    if(pcache->mutex==0)
//    {
//	pcache->mutex = 1;
//    }
//    else
//    {
//	return;
//    }
    pthread_mutex_lock(&pcache->mutex);

    if(pcache->pbuf!=NULL && pcache->cap>0)
    {
	if(pcache->roffset < 0) pcache->roffset = 0;
	if(pcache->roffset > pcache->woffset)
	    left = pcache->roffset - pcache->woffset;
	else
	    left = pcache->cap - (pcache->woffset - pcache->roffset);

	pcache->size += len;
	if(pcache->size > pcache->cap)
	    pcache->size = pcache->cap;
	while(len>0)
	{
	    if(len > pcache->cap - pcache->woffset)
	    {
		memcpy(pcache->pbuf+pcache->woffset,log+loff,pcache->cap - pcache->woffset);
		len -= (pcache->cap - pcache->woffset);
		loff += (pcache->cap - pcache->woffset);
		pcache->woffset = 0;
	    }
	    else
	    {
		memcpy(pcache->pbuf+pcache->woffset,log+loff,len);
		pcache->woffset = (pcache->woffset+len)%pcache->cap;
		break;
	    }
	}
	if(left < len)
	{
	    pcache->roffset = pcache->woffset;
	}
    }
    //printf("$$$$$$$ woff = %d,roff = %d\n",pcache->woffset,pcache->roffset);
//    pcache->mutex = 0;
    pthread_mutex_unlock(&pcache->mutex);

}

void aydebug_vindicate_logfile(void)
{
    ay_sdk_handle psdk = sdk_get_handle(0);
    if(psdk==NULL)
	return ;

    int  flen = 0;		
    char timebuf[64]={0};
    char jsonfile[256]={0};
    char debug_flag[256];

    st_ay_debug_ctrl *pdebug = &psdk->debug;
    snprintf(debug_flag,sizeof(debug_flag), "%s/ay_debug", psdk->devinfo.rw_path);
    snprintf(jsonfile,sizeof(jsonfile),"%s/ay.json",psdk->devinfo.rw_path);

    if(pdebug->log_cache.cap == 0)
    {
	pdebug->log_cache.pbuf = (char *)malloc(8096);
	if(pdebug->log_cache.pbuf)
	{
	    pdebug->log_cache.cap = 8096;
	    pdebug->log_cache.size = 0;
	    pdebug->log_cache.woffset = 0;
	    pdebug->log_cache.roffset = -1;
	    //pdebug->log_cache.mutex = 0;
	    pthread_mutex_init(&pdebug->log_cache.mutex, NULL);

	}
    }
    if(access(debug_flag,0))
    {
	/*
	if(pdebug->log_cache.cap > 0)
	{
	    free(pdebug->log_cache.pbuf);
	    pdebug->log_cache.pbuf = 0;
	    pdebug->log_cache.cap = 0;
	    pdebug->log_cache.size = 0;
	    pdebug->log_cache.woffset = 0;
	    pdebug->log_cache.roffset = -1;
	    pdebug->log_cache.mutex = 0;
	}
	*/

	remove(jsonfile);
    }
    else
    {
	Ulu_SDK_Refresh_Device_Info(jsonfile);
    }

    if(pdebug->en_flag != 1)
    {/* 0 or 2 */
	if(pdebug->en_flag && access(debug_flag,0))
	{// 2 and ay_debug file doesnot exist,we close sdk debug log.
	    pdebug->en_flag = 0;
	    fflush(pdebug->log_fd);
	    fclose(pdebug->log_fd);
	    pdebug->log_fd = NULL;
	    return;
	}
	else if(access(debug_flag,0))
	{// 0 and ay_debug file doesnot exist,we do nothing.
	    return;
	}
	else
	{// 0 and ay_debug file exist,we enable sdk debug log temporarily.
	    snprintf(pdebug->pathfile,sizeof(pdebug->pathfile), "%s/ay.log", psdk->devinfo.rw_path);
	    pdebug->max_fsize = 10*1024;
	    pdebug->en_flag = 2;
	}
    }
    if (pdebug->log_fd == NULL)
    {      
	pdebug->log_fd = fopen(pdebug->pathfile, "r+");
	if(pdebug->log_fd==NULL)
	{
	    pdebug->log_fd = fopen(pdebug->pathfile, "w+");
	}
	else
	{
	    fseek(pdebug->log_fd,0,SEEK_END);
	    flen = ftell(pdebug->log_fd);
	    if ( flen >= pdebug->max_fsize)
	    {
		fseek(pdebug->log_fd, 0, SEEK_SET);
	    }
	}
    }
    else
    {
	fflush(pdebug->log_fd);
	flen = ftell(pdebug->log_fd);
	if ( flen >= pdebug->max_fsize)
	{
	    fseek(pdebug->log_fd, 0, SEEK_SET);
	    aydebug_get_time(timebuf,sizeof(timebuf));
	    fprintf(pdebug->log_fd, "%s----------->\n", timebuf);
	}
	else if ( flen == -1 )
	{
	    fclose(pdebug->log_fd);
	    pdebug->log_fd = NULL;
	}
    }
}

void aydebug_printf(char *funNameStr, const char *buf, uint16 buflen)
{
    int i;
    DEBUGF("%s->len=%d  \n", funNameStr, buflen);
    for( i = 0; i < buflen ; i++ )
    {
        fprintf(stderr,"%-2x ", (uint8)(buf[i]));
    }
    DEBUGF("\n");
}

int ay_printf(int level,const char *format,...)
{
    ay_sdk_handle psdk = sdk_get_handle(0);
    if (psdk==NULL)
        return -1;

    int ret = 0;
    char timebuf[32]={0};
    char debug_flag[256];
    char log[2048]={0};
    va_list ap;

    va_start(ap,format);

    if(AYE_LOG_LEVEL_ERROR == level || AYE_LOG_LEVEL_TRACE==level)
    {
        char event[32] = {0};
        char msg[1024]={0};
        time_t t = time(NULL);
        //struct tm local;
        //localtime_r(&t,&local);
        //ret = strftime(log,sizeof(log), "[%F %T]", &local);
        //ret = snprintf(log,sizeof(log), "{{%s}}", "error");
        vsnprintf(log+ret,sizeof(log)-ret,format,ap);
        if(log[0] == '{')
        {
            sscanf(log,"{{%31[a-zA-Z0-9]}}%1023[^\n]",event,msg);
        }
        else
        {
            strcpy(event,"DeviceCommEvent");
            strcpy(msg,"Other|");
            sscanf(log,"%1010[^\n]",msg+strlen(msg));
        }
        aymonitor_get_uplog(t,event,msg,log,sizeof(log));
        aydebug_write_cache(&psdk->debug.log_cache,log);
    }

    snprintf(debug_flag,sizeof(debug_flag), "%s/ay_debug", psdk->devinfo.rw_path);

    if(psdk->debug.log_fd != NULL)
    {
        aydebug_get_time(timebuf,sizeof(timebuf));
        fprintf(psdk->debug.log_fd, "%s>",timebuf);
        ret = snprintf(log,sizeof(log),"%s>",timebuf);
        if(ret>0)
        {
            vsnprintf(log+ret,sizeof(log)-ret,format,ap);
            if(access(debug_flag,0)==0)
            {
                fprintf(stderr,"%s",log);
            }
        }
        ret = vfprintf(psdk->debug.log_fd, format, ap);
    }

    va_end(ap);
    return ret;
}

int aydebug_get_log(char log[],int size)
{
    memset(log,0,size);
    return aydebug_read_cache(log,size);
}

void set_thread_name(const char *desc)
{
#ifndef WIN32
    char threadname[64];
    
    memset(threadname, 0x0, sizeof(threadname));
    sprintf(threadname, "%s_%#x",desc, (unsigned int)pthread_self ());
    prctl(PR_SET_NAME, (unsigned long)threadname);
    DEBUGF ("starting thread %s - %ld\n", threadname,syscall(SYS_gettid));
#else
    DEBUGF ("starting thread %s\n", desc);
#endif
}

static char s_buffer[4096];
void ay_debug_printf(int level, const char* file, int line, const char *pszFormat, ...) {
    memset(s_buffer, 0x00, sizeof(s_buffer));
	//pthread_t tid = pthread_self();
    time_t curr_time;
    time(&curr_time);
    struct tm* p_local_time = localtime(&curr_time);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (level == AYE_LOG_LEVEL_ERROR) {
        sprintf(s_buffer, "\033[33m[%04d-%02d-%02d %02d:%02d:%02d.%03d][error][anyan][%s:%d]", 
            p_local_time->tm_year + 1900, p_local_time->tm_mon + 1, p_local_time->tm_mday,
            p_local_time->tm_hour, p_local_time->tm_min, p_local_time->tm_sec, (int)(tv.tv_usec / 1000), file, line);
    } else {
        sprintf(s_buffer, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][info][anyan][%s:%d]", 
            p_local_time->tm_year + 1900, p_local_time->tm_mon + 1, p_local_time->tm_mday,
            p_local_time->tm_hour, p_local_time->tm_min, p_local_time->tm_sec, (int)(tv.tv_usec / 1000), file, line);
    }

    va_list arg_ptr;
    va_start(arg_ptr, pszFormat);
    vsnprintf(s_buffer + strlen(s_buffer), sizeof(s_buffer) - strlen(s_buffer) - 10, pszFormat, arg_ptr);
    va_end(arg_ptr);

    //清掉颜色
    snprintf(s_buffer + strlen(s_buffer), sizeof(s_buffer) - strlen(s_buffer) - 1, "\033[0m");

    printf(s_buffer);
}
