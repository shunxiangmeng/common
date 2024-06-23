#ifndef __AY_UTIL_H__
#define __AY_UTIL_H__

#include <time.h>
#include "typedefine.h"

void ayutil_sleep(int ms);
void ayutil_save_version(char *path,char *fname);
void ayutil_save_file(char *path,char *fname,const char *content,int size);
int  ayutil_read_file(char *path,char *fname,char content[],int size);
int  ayutil_cost_mseconds(struct timespec t1,struct timespec t2);
int  ayutil_wait_sem(sem_t *wSem,  int msec);
int  ayutil_query_rate_index(int  rate);

int ayutil_getaddrinfo_bysockaddr(const char *host,unsigned short port,struct sockaddr_in *servaddr);
int ayutil_getaddrinfo_byip(const char *host,unsigned short port,char ip[],int size);
unsigned int ayutil_get_host_ip(int sock);
unsigned int ayutil_get_local_ip(const char *first,const char *second);
int ayutil_init_udp(int local_port);
int ayutil_send_udp(int fd, char *pdata, int len,  uint32 ip_addr, uint16 port);
int ayutil_tcp_client(char* host, unsigned short port,int timeout);
int ayutil_tcp_client2(unsigned int ip, unsigned short port,int timeout);
int ayutil_tcp_server(unsigned int ip,unsigned short port,int timeout);

int ayutil_add_host(const char *host,unsigned int ip);
int ayutil_del_host(const char *host);
int ayutil_query_host(const char *host,unsigned int *ip);
int ayutil_pthread_create (pthread_t *thread, const pthread_attr_t *pattr, void *(*start_routine) (void *), void *arg);

#endif
