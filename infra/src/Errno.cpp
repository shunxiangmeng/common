/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Errno.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 17:15:24
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Errno.h"

namespace infra {

int32_t lastErrno(bool network_error) {
#ifdef _WIN32
    return network_error ? WSAGetLastError() : GetLastError();
#else
    return errno;
#endif
}

}