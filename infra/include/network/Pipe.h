/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Pipe.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-19 21:38:03
 * Description :  使用socket实现的pipe
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "TcpSocket.h"

namespace infra {
class Pipe {
public:
    Pipe();
    ~Pipe();

    bool open();
    void close();
    int32_t write(const char *buffer, int32_t length);
    int32_t read(char *buffer, int32_t length);
    int32_t readFd() const;
    int32_t writeFd() const;
private:
    std::shared_ptr<TcpSocket> pipe_fd_[2];
};
}