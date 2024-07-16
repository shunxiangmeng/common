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
#include <cstdio>
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
FileLogChannel::FileLogChannel(const std::string &path, const std::string &name) : LogChannel(name), 
    path_(path), file_max_size_(4 * 1024 * 1024) {
    if (path_.empty() || path_ == "") {
        return;
    }
    if (path_.at(path_.length() - 1) == '/') {
        path_ = path_.substr(0, path_.length() - 1);
    }
    std::string log_file_name = getLogfileName(true);
    #ifndef _WIN32
        // 创建文件夹
        File::createPath(log_file_name.data(), S_IRWXO | S_IRWXG | S_IRWXU);
    #else
        File::createPath(log_file_name.data(), 0);
    #endif
    fstream_.open(log_file_name.data(), std::ios::out | std::ios::app);
    if (!fstream_.is_open()) {
        printf("[%s:%d]open log file %s error\n", __FILE__, __LINE__, log_file_name.data());
    } else {
        std::string log_start_str = "\n............" + Logger::instance().printTime() + "............\n";
        fstream_ << log_start_str;
        fstream_.flush();
    }
}

FileLogChannel::~FileLogChannel() {
    fstream_.close();
}

void FileLogChannel::write(const std::vector<std::shared_ptr<LogContent>> &content) {
    if (fstream_.is_open()) {
        for (size_t i = 0; i < content.size(); i++) {
            if (!maybeSaveLogfile(content[i]->content.size())) {
                return;
            }
            fstream_ << content[i]->content;
        }
        fstream_.flush();
    }
}

std::string FileLogChannel::getLogfileName(bool current) {
    if (current) {
        return path_ + "/current.log";
    }

    time_t now = time(NULL);
    struct tm* local;
    local = localtime(&now);
    char time_str[64] = { 0 };
    snprintf(time_str, sizeof(time_str), "/log%d-%02d-%02d_%02d_%02d_%02d.log", 
        1900 + local->tm_year, 1 + local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
    return path_ + time_str;
}

bool FileLogChannel::maybeSaveLogfile(int32_t next_size) {
    int32_t size = fstream_.tellp();
    if (size + next_size >= file_max_size_) {
        fstream_.flush();
        fstream_.close();

        std::string current_log_file_name = getLogfileName(true);
        (void)std::rename(current_log_file_name.data(), getLogfileName(false).data());

        #ifndef _WIN32
            // 创建文件夹
            File::createPath(current_log_file_name.data(), S_IRWXO | S_IRWXG | S_IRWXU);
        #else
            File::createPath(current_log_file_name.data(), 0);
        #endif
        fstream_.open(current_log_file_name.data(), std::ios::out | std::ios::app);
        if (!fstream_.is_open()) {
            printf("[%s:%d]open log file %s error\n", __FILE__, __LINE__, current_log_file_name.data());
            return false;
        }
    }
    return true;
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
    char buffer[4096] = { 0 };
    if (vsnprintf(buffer, sizeof(buffer), fmt, ap) > 0) {
        std::string content;
        std::string file_name = file;

        {
            /* 去掉文件的全路径，只保留文件名 */
            file_name = file_name.substr(file_name.rfind(DIR_SEPARATOR) + 1, -1);
        }

        content += printTime();
        content += std::string("[") + std::to_string(getCurrentThreadId()) + std::string("]");  /* 线程ID的hash值 */
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
    va_list ap;
    va_start(ap, fmt);
    char buffer[4096] = { 0 };
    if (vsnprintf(buffer, sizeof(buffer), fmt, ap) > 0) {
        infra::Logger::instance().printLog((infra::LogLevel)level, file, line, buffer);
    }
    va_end(ap);
}
}