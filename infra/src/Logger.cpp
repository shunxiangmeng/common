/********************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :   Logger.cpp
 * Author      :   mengshunxiang 
 * Data        :   2024-02-23 23:15:15
 * Description :   None
 * Others      : 
 ********************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <time.h>
#include <chrono>
#include "infra/include/Logger.h"
#include "infra/include/File.h"
#include "infra/include/Utils.h"
#if defined(_WIN32)
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#endif

namespace infra {

#if defined _WIN32 ||defined _WIN64
#define DIR_SEPARATOR "\\"
#else
#define DIR_SEPARATOR "/"
#endif


//控制台日志输出
ConsoleLogChannel::ConsoleLogChannel(const std::string &name) : LogChannel(name) {
}

void ConsoleLogChannel::setConsoleColour(LogLevel level) {
#ifdef _WIN32
    #define CLEAR_COLOR 7
    static const WORD LOG_CONST_TABLE[][3] = {
        {0xC7, 0x0C , 'E'},  //红底灰字，黑底红字
        {0xE7, 0x0E , 'W'},  //黄底灰字，黑底黄字
        {0xB7, 0x0A , 'I'},  //天蓝底灰字，黑底绿字
        {0xA7, 0x0B , 'D'},  //绿底灰字，黑底天蓝字
        {0x97, 0x0F , 'T'}   //蓝底灰字，黑底白字，window console默认黑底  
    };
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == 0) {
        return;
    }
    SetConsoleTextAttribute(handle, LOG_CONST_TABLE[int(level)][1]);
#else
#define CLEAR_COLOR "\033[0m"
    static const char* LOG_CONST_TABLE[] = {"\033[31;1m", "\033[33;1m", "\033[32m", "\033[0m", "\033[0m"};
    std::cout << LOG_CONST_TABLE[int(level)];
#endif
}

void ConsoleLogChannel::clearConsoleColour() {
#ifdef _WIN32
    //todo
#else
#define CLEAR_COLOR "\033[0m"
    std::cout << CLEAR_COLOR;
#endif
}

void ConsoleLogChannel::write(const std::vector<std::shared_ptr<LogContent>> &content) {
    for (size_t i = 0; i < content.size(); i++) {
        setConsoleColour(content[i]->level);
        std::cout << content[i]->content;
        clearConsoleColour();
    }
}


//文件日志输出
FileLogChannel::FileLogChannel(const std::string &path, const std::string &name) : LogChannel(name), path_(path) {
    if (path_.empty() || path_ == "") {
        return;
    }
    #ifndef _WIN32
        // 创建文件夹
        File::createPath(path_.data(), S_IRWXO | S_IRWXG | S_IRWXU);
    #else
        File::createPath(path_.data(), 0);
    #endif
    fstream_.open(path_.data(), std::ios::out | std::ios::app);
    if (!fstream_.is_open()) {
        printf("open log file %s error\n", path_.data());
    }
}

FileLogChannel::~FileLogChannel() {
    fstream_.close();
}

void FileLogChannel::write(const std::vector<std::shared_ptr<LogContent>> &content) {
    if (fstream_.is_open()) {
        for (size_t i = 0; i < content.size(); i++) {
            fstream_ << content[i]->content;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
Logger& Logger::instance() {
    static std::shared_ptr<Logger> slogger = std::make_shared<Logger>(LogLevelTrace);
    return *slogger;
}

Logger::Logger(LogLevel level) : level_(level), running_(true) {
    thread_ = std::make_shared<std::thread>([this]() {run(); });
}

Logger::~Logger() {
    flush();
    std::lock_guard<decltype(logChannelsMutex_)> lock(logChannelsMutex_);
    logChannels_.clear();
    running_ = false;
    if (thread_->joinable()) {
        thread_->join();
    }
}

void Logger::addLogChannel(const std::shared_ptr<LogChannel>& channel) {
    std::lock_guard<decltype(logChannelsMutex_)> lock(logChannelsMutex_);
    logChannels_.push_back(channel);
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::flush() {
    std::vector<std::shared_ptr<LogContent>> tmp;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        content_.swap(tmp);
    }

    //std::lock_guard<decltype(logChannelsMutex_)> lock(logChannelsMutex_);
    for (auto it : logChannels_) {
        it->write(tmp);
    }
}

void Logger::run() {
    while (running_) {
        semaphore_.wait();
        flush();
        //优化快速写日志的效率
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string Logger::printTime() {
    uint64_t timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    struct tm now;
#ifdef _WIN32
    uint64_t milli = timestamp + 8 * 60 * 60 * 1000;  //time zone
    auto mTime = std::chrono::milliseconds(milli);
    auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
    auto tt = std::chrono::system_clock::to_time_t(tp);
    gmtime_s(&now, &tt);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    now = *localtime(&tv.tv_sec);
#endif

    char buffer[64] = { 0 };
    snprintf(buffer, sizeof(buffer), "[%4d-%02d-%02d %02d:%02d:%02d.%03d]", 
        now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, int(timestamp % 1000));
    return buffer;
}

void Logger::writeLog(LogLevel level, const std::string &content) {
#if 1
    std::shared_ptr<LogContent> log(new LogContent(level, content));
    {
        std::lock_guard<std::mutex> guard(mutex_);
        content_.push_back(log);
    }
    semaphore_.post();
#else
    std::cout << content;
#endif
}

static const char* sLogLevelString[] = {"[error]", "[warn]", "[info]", "[debug]", "[trace]"};

void Logger::printLog(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level > level_) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    char buffer[1024] = { 0 };
    if (vsnprintf(buffer, sizeof(buffer), fmt, ap) > 0) {
        std::string content;
        std::string file_name = file;

        {
            /* 去掉文件的全路径，只保留文件名 */
            file_name = file_name.substr(file_name.rfind(DIR_SEPARATOR) + 1, -1);
        }

        content += printTime();
        content += std::string("[") + std::to_string(getCurrentThreadId()) + std::string("]");  //线程ID的hash值
        content += sLogLevelString[int(level)];
        content += std::string("[") + file_name + std::string(":") + std::to_string(line) + std::string("]");
        content += buffer;
        writeLog(LogLevel(level), content);
    }
    va_end(ap);
}

}  //namespace


extern "C" {
void printflog_for_c(int level, const char *file, int line, const char *fmt, ...) {
    infra::Logger::instance().printLog((infra::LogLevel)level, file, line, fmt);
}
}