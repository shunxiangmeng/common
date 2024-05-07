/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Errno.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-18 17:15:18
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#endif

namespace infra {

#ifdef _WIN32

#else

#ifndef STRERROR
#define STRERROR strerror(errno)
#endif

#endif

int32_t lastErrno(bool network_error = false);
}