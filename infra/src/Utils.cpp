/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Utils.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 15:49:45
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/Utils.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif // _WIN32

namespace infra {

uint32_t htonl(uint32_t value) {
    return ::htonl(value);
}

uint16_t htons(uint16_t value) {
    return ::htons(value);
}

uint32_t ntohl(uint32_t value) {
    return ::ntohl(value);
}

uint16_t ntohs(uint16_t value) {
    return ::ntohs(value);
}

int32_t getCurrentThreadId() {
#ifdef _WIN32
    return  GetCurrentProcessId();
    //#define getCurrentThreadId() std::this_thread::get_id()
#else
    return getpid();
#endif
}

}