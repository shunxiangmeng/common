/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Logger_c.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-18 20:52:47
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once

typedef enum {
    LogLevelError = 0,
    LogLevelWarn,
    LogLevelInfo,
    LogLevelDebug,
    LogLevelTrace
} LogLevel;

void printflog_for_c(int level, const char *file, int line, const char *fmt, ...);

#define printLogf(level, ...) printflog_for_c(level, __FILE__, __LINE__, ##__VA_ARGS__)

#define errorf(...) printLogf(LogLevelError, ##__VA_ARGS__)
#define warnf(...)  printLogf(LogLevelWarn, ##__VA_ARGS__)
#define infof(...)  printLogf(LogLevelInfo, ##__VA_ARGS__)
#define debugf(...) printLogf(LogLevelDebug, ##__VA_ARGS__)
#define tracef(...) printLogf(LogLevelTrace, ##__VA_ARGS__)
