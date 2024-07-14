#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <sys/types.h>
#endif
#include "infra/include/Logger.h"

namespace infra {

#ifdef _WIN32
int close(int fd);
int ioctl(int fd, long cmd, u_long *ptr);
#endif

#define CHECK_FD(fd) if (fd < 0) { errorf("fd_ < 0\n"); return false; }

}
