/********************************************************************
 * Copyright(c) 2024 ulucu technology
 * 
 * Author:     mengshunxiang
 * Date:       2024-01-22
 * Description: 
 * Others:
 *******************************************************************/
#pragma once
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include "infra/include/Semaphore.h"
#include "infra/include/Utils.h"

namespace infra {

typedef enum {
    LogLevelError = 0,
    LogLevelWarn,
    LogLevelInfo,
    LogLevelDebug,
    LogLevelTrace
} LogLevel;

typedef struct LogContent {
    LogLevel level;
    std::string content;
    LogContent(LogLevel l, const std::string& c) : level(l), content(c) {}
} LogContent;

class LogChannel : public noncopyable {
public:
    LogChannel(const std::string &name) : name_(name) {};
    virtual ~LogChannel() {};
    virtual void write(const std::vector<std::shared_ptr<LogContent>> &content) = 0;
protected:    
    std::string name_;
};


class ConsoleLogChannel : public LogChannel {
public:
    ConsoleLogChannel(const std::string &name = "console");
    virtual ~ConsoleLogChannel() override = default;
    virtual void write(const std::vector<std::shared_ptr<LogContent>> &content) override;
private:
    void setConsoleColour(LogLevel level);
    void clearConsoleColour();
};


class FileLogChannel : public LogChannel {
public:
    FileLogChannel(const std::string &path, const std::string &name = "file");
    virtual ~FileLogChannel() override;
    virtual void write(const std::vector<std::shared_ptr<LogContent>> &content) override;
private:
    std::string path_;
    std::ofstream fstream_;
};


class Logger : public noncopyable {
public:
    static Logger &instance();
    Logger(LogLevel level = LogLevelInfo);
    ~Logger();
    void addLogChannel(const std::shared_ptr<LogChannel>& channel);
    void setLevel(LogLevel level);
    void printLog(LogLevel level, const char *file, int line, const char *fmt, ...);
private:
    void run();
    void flush();
    void writeLog(LogLevel level, const std::string &content);
    std::string printTime();
private:
    LogLevel level_;
    bool running_;
    Semaphore semaphore_;
    std::shared_ptr<std::thread> thread_;
    std::vector<std::shared_ptr<LogChannel>> logChannels_;
    std::mutex logChannelsMutex_;
    std::vector<std::shared_ptr<LogContent>> content_;
    std::mutex mutex_;
};

}  //namespace infra


#define printLogf(level, ...) infra::Logger::instance().printLog(level, __FILE__, __LINE__, ##__VA_ARGS__)

#define errorf(...) printLogf(infra::LogLevelError, ##__VA_ARGS__)
#define warnf(...)  printLogf(infra::LogLevelWarn, ##__VA_ARGS__)
#define infof(...)  printLogf(infra::LogLevelInfo, ##__VA_ARGS__)
#define debugf(...) printLogf(infra::LogLevelDebug, ##__VA_ARGS__)
#define tracef(...) printLogf(infra::LogLevelTrace, ##__VA_ARGS__)

#define LOG_CHECK(condition) (condition) ? (void)0 : errorf("check %s failed\n", #condition)
#define LOG_CHECK_EQ(v1, v2) ((v1) == (v2)) ? (void)0 : errorf("check %s == %s failed\n", #v1, #v2)
#define LOG_CHECK_LE(v1, v2) ((v1) <= (v2)) ? (void)0 : errorf("check %s <= %s failed\n", #v1, #v2)
#define LOG_CHECK_LT(v1, v2) ((v1) < (v2)) ? (void)0 : errorf("check %s < %s failed\n", #v1, #v2)
#define LOG_CHECK_NE(v1, v2) ((v1) != (v2)) ? (void)0 : errorf("check %s != %s failed\n", #v1, #v2)
#define LOG_CHECK_GE(v1, v2) ((v1) >= (v2)) ? (void)0 : errorf("check %s >= %s failed\n", #v1, #v2)
#define LOG_CHECK_GT(v1, v2) ((v1) > (v2)) ? (void)0 : errorf("check %s > %s failed\n", #v1, #v2)
