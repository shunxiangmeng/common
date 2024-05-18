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
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/syscall.h>        //gettid
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
    return  GetCurrentThreadId();
    //#define getCurrentThreadId() std::this_thread::get_id()
#else
    return (int32_t)syscall(SYS_gettid);
#endif
}

std::string exePath(bool isExe /*= true*/) {
    char buffer[256 + 1] = {0};
    int n = -1;
#if defined(_WIN32)
    n = GetModuleFileNameA(isExe ? nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
    n = sizeof(buffer);
    if (uv_exepath(buffer, &n) != 0) {
        n = -1;
    }
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
    std::string file_path;
    if (n <= 0) {
        file_path = "./";
    } else {
        file_path = buffer;
    }

#if defined(_WIN32)
    //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : file_path) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif //defined(_WIN32)
    return file_path;
}

}