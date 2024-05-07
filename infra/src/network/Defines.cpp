#include "infra/include/network/Defines.h"

namespace infra {

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")

int close(int fd) {
    return closesocket(fd);
}

int ioctl(int fd, long cmd, u_long *ptr) {
    return ioctlsocket(fd, cmd, ptr);
}

#endif
}