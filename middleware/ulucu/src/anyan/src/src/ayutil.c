#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WIN32
#include <netinet/tcp.h>
#include <net/if.h>
#endif

#include "define.h"
#include "protocol_device.h"
#include "ayutil.h"
#include "ay_sdk.h"
#include "aydevice2.h"
#include "version.h"

/*************************** General Utilities **************************/
void ayutil_sleep(int ms)
{
#if 1
    usleep(1000*ms);
#else
    struct timeval timeout;
    timeout.tv_sec = ms/1000;
    timeout.tv_usec = (ms%1000)*1000;
    select(0, NULL, NULL, NULL, &timeout);
#endif
}

int ayutil_cost_mseconds(struct timespec t1,struct timespec t2)
{
    return ((t2.tv_sec*1000 + t2.tv_nsec/(1000*1000)) - (t1.tv_sec*1000 + t1.tv_nsec/(1000*1000)));
}

int ayutil_read_file(char *path,char *fname,char content[],int size)
{
    char tmp_path[128];
    FILE  *fp = NULL;
	int ret;
    if (path != NULL && fname != NULL && content != NULL)
    {
        snprintf(tmp_path,sizeof(tmp_path),"%s/%s", path,fname);
        fp = fopen(tmp_path, "rb");
        if (fp != NULL)
        {
            ret = fread(content, 1, size, fp);
            fclose(fp);
            if (ret != size)
                return -1;
            else
                return 0;
        }
        return -1;
    }
    return -1;
}

void ayutil_save_file(char *path, char *fname, const char *content, int size)
{
    char tmp_path[128];
    FILE  *fp = NULL;
    if (path != NULL && fname != NULL && content != NULL)
    {
        snprintf(tmp_path,sizeof(tmp_path),"%s/%s", path,fname);
        fp = fopen(tmp_path, "wb+");
        if (fp != NULL)
        {
            fwrite(content, 1, size, fp);
            fclose(fp);
        }
    }
}

void ayutil_save_version(char *path,char *fname)
{
    uint16   vs[4];
    char     buf[256];

    vs[0] = COMPILE_COMMAND;
    vs[1] = (GCC_VERSION_1 << 12) + (GCC_VERSION_2 << 8) + (GCC_VERSION_3 << 4)+GCC_TYPE_LIB;
    vs[2] = (YEAT << 8) + MONTH;
    vs[3] = (DAYS << 8) + COUNT;
    memset(buf, 0, sizeof(buf));

    snprintf(buf,sizeof(buf), "%d.%d.%d.%d(%d-%d-%d[%d])\r\nRelease: %s\r\n", 
	    vs[0],vs[1],vs[2],vs[3],YEAT,MONTH,DAYS,COUNT,RVERSION);
    ayutil_save_file(path,fname,buf,strlen(buf));
}

int  ayutil_wait_sem(sem_t *wSem,  int  ms)
{
    int sts;
    struct timeval now;
    struct timespec tout;

    gettimeofday(&now,NULL);
    tout.tv_sec = now.tv_sec + ms/1000;
    tout.tv_nsec = now.tv_usec*1000 + (ms%1000)*1000*1000;

    /* Try to lock Semaphore */
    sts = sem_timedwait(wSem, &tout);
    return sts;
}

int  ayutil_query_rate_index(int  rate)
{
    int index = 0;
    if(rate <= UPLOAD_RATE_1)
	index = 0;
    else if(rate <= UPLOAD_RATE_2)
	index = 1;
    else if(rate <= UPLOAD_RATE_3)
	index = 2;
    else //if(rate <= UPLOAD_RATE_4)
	index = 3;
    return index;
}


/*************************** Network Utilities **************************/
static int ay_set_socket_unblocking(int sock)
{
#ifdef WIN32
    int iResult = 0;
    u_long iMode = 1;
    iResult = ioctlsocket(sock, FIONBIO, &iMode);
    if (iResult != NO_ERROR) {
        ERRORF("ioctlsocket failed with error: %ld\n", iResult);
        return -1;
    }
    return 0;
#else
    int flags;
    flags = fcntl(sock, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(sock,F_SETFL,flags) == -1) {
        ERRORF("fcntl fail to set noblock %s\n", strerror(errno));
        return -1;
    }
    return 0;
#endif
}

static int ay_set_socket_block(int sockfd,int timeout,int keepidle)
{
    if(sockfd > 0 && timeout > 0)
    {
	struct timeval tout;
	tout.tv_sec = timeout;
	tout.tv_usec = 0;
	setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&tout,sizeof(tout));
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tout,sizeof(tout));

	if(keepidle > 0)
	{
	    int keepalive = 1;
	    //int keepidle = 30;
	    int keepinterval = 1;
	    int keepcount = 3;
	    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
	    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle));
	    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval));
	    setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount));
	}
    }
    return -1;
}

unsigned int ayutil_get_host_ip(int sock)
{
    struct sockaddr_in sin;
    socklen_t addr_len = sizeof(sin);

    if(getsockname(sock,(struct sockaddr*)&sin,&addr_len))
    {
	ERRORF("getsockname fail:%s\n",strerror(errno));
	return 0;
    }

    DEBUGF("host ip:%s\n", inet_ntoa(sin.sin_addr));
    return (*(unsigned int*)&sin.sin_addr);
}

unsigned int ayutil_get_local_ip(const char *first,const char *second)
{
#ifdef WIN32
    HOSTENT* localHost;
    char* localIP;

    // Get the local host information
    localHost = gethostbyname("");
    localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);
    DEBUGF("local IP :%s\n", localIP);
    return (*(unsigned int*)*localHost->h_addr_list);
#else
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) 
        return 0;

    strcpy((char*)&ifr.ifr_name, first);
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0 && second!=NULL)
    {
        memset(&ifr,0,sizeof(ifr));
        strcpy((char*)&ifr.ifr_name, second);
        if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
        {
            close(sock);
            return 0;
        }
    }
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    close(sock);
    DEBUGF("local[%s] IP :%s\n", ifr.ifr_name,inet_ntoa(sin.sin_addr));
    return (*(unsigned int*)&sin.sin_addr);
#endif
}

int ayutil_init_udp(int local_port)
{
    struct sockaddr_in addr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        ERRORF("socket error:%s\n", strerror(errno));
        return -1;
    }
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    addr.sin_addr.s_addr = 0;
    bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

    if (ay_set_socket_unblocking(sockfd) < 0) {
        closesocket(sockfd);
        return -1;
    }
    if (sockfd > 0 ) {
        DEBUGF("udp bind port %d ok, sockfd = %d\n", local_port, sockfd);
    }
    return sockfd;
}

int ayutil_getaddrinfo_byip(const char *host,unsigned short port,char ip[],int size)
{
#if USE_GETHOSTBYNAME
    struct hostent* ht = NULL; 
    ht = gethostbyname(host);

    if (ht)
    {
	DEBUGF("gethostbyname IP: %s\n", inet_ntoa(*(struct in_addr*)ht->h_addr_list[0]));
	strncpy(ip, inet_ntoa(*(struct in_addr*)ht->h_addr_list[0]),size);
	return 0;
    }
    ERRORF("gethostbyname %s fail!\n",host);
    return -1;
#else
    struct addrinfo hints, *res;
    char service[32]={0};
    int ret = 0;

    memset(&hints,0,sizeof(hints));
    //hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    //hints.ai_socktype = SOCK_STREAM;
    snprintf(service,sizeof(service),"%d",port);

    ret = getaddrinfo(host,service,&hints,&res);
    if(ret < 0)
	ret = getaddrinfo(host,"http",&hints,&res);
    if(ret)
    {
	ERRORF("getaddrinfo %s fail!\n",host);
	return -1;
    }
    else
    {
	struct addrinfo *ptr;
	for(ptr=res;ptr;ptr=ptr->ai_next)
	{
	    inet_ntop(ptr->ai_family,&((struct sockaddr_in*)(ptr->ai_addr))->sin_addr,ip,size);
	}
	freeaddrinfo(res);
    }
    return 0;
#endif
}

int ayutil_getaddrinfo_bysockaddr(const char *host,unsigned short port,struct sockaddr_in *servaddr)
{
#if USE_GETHOSTBYNAME
    struct hostent* ht = NULL; 
    ht = gethostbyname(host);

    if (ht)
    {
	//DEBUG(" IP: %s\n", inet_ntoa(*(struct in_addr*)ht->h_addr_list[0]));
	servaddr->sin_family = AF_INET;
	//servaddr->sin_addr = *(struct in_addr*)ht->h_addr_list[0];
	memcpy((char *) &servaddr->sin_addr, (char *) ht->h_addr_list[0], ht->h_length);
	servaddr->sin_port = htons(port);
	return 0;
    }
    ERRORF("gethostbyname %s fail!\n",host);
    return -1;
#else
    struct addrinfo hints, *res;
    char service[32]={0};
    int ret = 0;

    memset(&hints,0,sizeof(hints));
    //hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_INET;
    //hints.ai_socktype = SOCK_STREAM;
    snprintf(service,sizeof(service),"%d",port);

    ret = getaddrinfo(host,service,&hints,&res);
    if(ret < 0)
	ret = getaddrinfo(host,"http",&hints,&res);
    if(ret)
    {
	ERRORF("getaddrinfo %s fail!\n",host);
	return -1;
    }
    else
    {
	struct addrinfo *ptr;
	for(ptr=res;ptr;ptr=ptr->ai_next)
	{
	    memcpy(servaddr,ptr->ai_addr,sizeof(struct sockaddr));
	    servaddr->sin_port = htons(port);
	    break;
	    // char str[64]={0};
	    //servaddr = (struct sockaddr_in*)ptr->ai_addr;
	    //fprintf(stdout,"\taddress: %s\n", inet_ntop(ptr->ai_family,&servaddr->sin_addr,str,sizeof(str)));
	}
	freeaddrinfo(res);
    }
    return 0;
#endif
}

int ayutil_tcp_client(char* host, unsigned short port, int timeout)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if (ayutil_getaddrinfo_bysockaddr((const char *)host,(unsigned short)port,&server_addr)<0)
    {
        ERRORF("get %s addrinfo fail!\n",host);
        return -1;
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERRORF("socket error:%s\n", strerror(errno));
        return -1;
    }
    if (timeout > 0)
    {
        ay_set_socket_block(sockfd, timeout, 40);
    }
    else if (ay_set_socket_unblocking(sockfd) < 0)
    {
        closesocket(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)(&server_addr),sizeof(struct sockaddr)) != 0)
    {
        ERRORF("connect %s:%hu err:%s\n",host,port,strerror(errno));
        closesocket(sockfd);
        return  -1;
    }
    return sockfd;
}

int ayutil_tcp_server(unsigned int ip,unsigned short port,int timeout)
{
    int sockfd = -1;
    struct sockaddr_in server_addr;

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);// htonl(ip)
    server_addr.sin_port = htons(port);

    DEBUGF("port = %hu,sin_port = %hu\n",port,server_addr.sin_port);
    if((sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
	ERRORF("socket error:%s\n", strerror(errno));
	return -1;
    }
    if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr))<0)
    {
	ERRORF("tcp server bind %hu fail:%s\n",port,strerror(errno));
	closesocket(sockfd);
	return -1;
    }
    int reuse = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,sizeof(reuse));
    
    if(timeout > 0)
    {
	ay_set_socket_block(sockfd,timeout,40);
    }
    else if(ay_set_socket_unblocking(sockfd) < 0)
    {
	closesocket(sockfd);
	return -1;
    }
    listen(sockfd,10);
    return sockfd;
}

int ayutil_add_host(const char *host,unsigned int ip)
{
    if(host==NULL) return -1;

    char fname[256];
    FILE *fp = NULL;

    snprintf(fname,sizeof(fname),"%s/ay_host",sdk_get_handle(0)->devinfo.rw_path);
    if(access(fname,0)) fp = fopen(fname,"w+");
    else fp = fopen(fname,"r+");

    if(fp!=NULL)
    {
	char content[8192]={0};
	char item[128];
	long offset = 0;
	struct in_addr addr;

	addr.s_addr = ip;
	snprintf(item,sizeof(item),"%s\t%s\n",host,inet_ntoa(addr));

	offset = fread(content,1,sizeof(content)-1,fp);
	if(offset > 0)
	{
	    fclose(fp);
	    fp = fopen(fname,"w+");
	    char *p = strstr(content,host);
	    if(p!=NULL)
	    {
		char *q = strstr(p,"\n");
		if(q!=NULL)
		{
		    memmove(p,q+1,strlen(q+1)+1);// include '\0'
		}
		else {*p = '\0';}

		strcat(content,item);
	    }
	    else if(offset+strlen(item) < sizeof(content))
	    {
		strcat(content,item);
	    }
	    else
	    {
		strcpy(content,item);
	    }
	}
	else
	{
	    strcpy(content,item);
	}
	fwrite(content,1,strlen(content),fp);
	fclose(fp);
	return 0;
    }
    return -1;
}

int ayutil_query_host(const char *host,unsigned int *ip)
{
    if(host==NULL || ip==NULL) return -1;

    int ret = -1;
    char fname[256];
    FILE *fp = NULL;

    snprintf(fname,sizeof(fname),"%s/ay_host",sdk_get_handle(0)->devinfo.rw_path);
    fp = fopen(fname,"r");

    if(fp!=NULL)
    {
	char content[8192]={0};
	long offset = 0;

	offset = fread(content,1,sizeof(content)-1,fp);
	if(offset > 0)
	{
	    char *p = strstr(content,host);
	    if(p!=NULL)
	    {
		char *q = strstr(p,"\t");
		char *s = strstr(q,"\n");
		if(q!=NULL && s!=NULL)
		{
		    char item[32]={0};
		    memcpy(item,q+1,s-q-1);
		    *ip = inet_addr(item);
		    ret = 0;
		}
	    }
	}
	else 
	{
	    ret = -1;
	}

	fclose(fp);
    }
    if(ret < 0)
    {
	st_ay_net_addr addr;
	if(aydevice2_nslookup_host(host,&addr) == 0)
	{
	    *ip = inet_addr(addr.ipstr);
	    ret = 0;
	}
	else if(strcmp(host,AYENTRY_SERVER_PUBLIC) == 0)
	{
	    *ip = inet_addr("115.159.17.225");
	    ret = 0;
	}
	else if(strcmp(host,AYENTRY_SERVER_CONFIG) == 0)
	{
	    *ip = inet_addr("115.159.111.107");
	    ret = 0;
	}
    }
    return ret;
}

int ayutil_del_host(const char *host)
{
    if(host==NULL) return -1;

    char fname[256];
    FILE *fp = NULL;

    snprintf(fname,sizeof(fname),"%s/ay_host",sdk_get_handle(0)->devinfo.rw_path);
    fp = fopen(fname,"r+");

    if(fp!=NULL)
    {
	char content[8192]={0};
	long offset = 0;

	offset = fread(content,1,sizeof(content)-1,fp);
	fclose(fp);

	if(offset > 0)
	{
	    char *p = strstr(content,host);
	    if(p!=NULL)
	    {
		char *q = strstr(p,"\n");
		if(q!=NULL)
		{
		    memmove(p,q+1,strlen(q+1)+1);// include '\0'
		}
		else { *p = '\0'; }

		fp = fopen(fname,"w+");
		fwrite(content,1,strlen(content),fp);
		fclose(fp);
	    }
	}
	return 0;
    }
    return -1;
}

int ayutil_tcp_client2(unsigned int ip, unsigned short port,int timeout)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if((sockfd=socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
	ERRORF("socket error:%s\n", strerror(errno));
	return -1;
    }
    if(timeout > 0)
    {
	ay_set_socket_block(sockfd,timeout,40);
    }
    else if(ay_set_socket_unblocking(sockfd) < 0)
    {
	closesocket(sockfd);
	return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ip;
    server_addr.sin_port = htons(port);
    if(connect(sockfd, (struct sockaddr *)(&server_addr),sizeof(struct sockaddr)) != 0)
    {
	ERRORF("connect %s:%hu err:%s\n",inet_ntoa(server_addr.sin_addr),port,strerror(errno));
	closesocket(sockfd);
	return  -1;
    }
    return sockfd;
}

int ayutil_pthread_create (pthread_t *thread, const pthread_attr_t *pattr, void *(*start_routine) (void *), void *arg)
{/* stack size 2M */
    size_t stacksize = 2048 * 1024;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stacksize);

    if (!pattr)
    {
		pattr = &attr;
		printf ("stacksize is [%u]\n", stacksize);
    }

    return pthread_create (thread, pattr, start_routine, arg);
}
