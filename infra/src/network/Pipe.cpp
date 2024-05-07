/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Pipe.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-19 21:39:38
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Pipe.h"
#include "AcceptSocket.h"
#include "../Logger.h"

namespace infra {

Pipe::Pipe() {
}

Pipe::~Pipe() {
    close();
}

bool Pipe::open() {
    AcceptSocket acceptor;
    acceptor.listen("127.0.0.1", 0);
    acceptor.setNoblocked(false);

    pipe_fd_[0] = std::make_shared<TcpSocket>();
    pipe_fd_[0]->connect("127.0.0.1", acceptor.getLocalPort(), false);

    pipe_fd_[1] = acceptor.accept();

    //infof("pipe fd %d %d\n", pipe_fd_[0]->getHandle(), pipe_fd_[1]->getHandle());
    return true;
}

void Pipe::close() {
    pipe_fd_[0].reset();
    pipe_fd_[1].reset();
}

int32_t Pipe::write(const char *buffer, int32_t length) {
    return pipe_fd_[0]->send(buffer, length);
}

int32_t Pipe::read(char *buffer, int32_t length) {
    return pipe_fd_[1]->recv(buffer, length);
}

int32_t Pipe::writeFd() const { 
    return pipe_fd_[0]->getHandle(); 
}

int32_t Pipe::readFd() const { 
    return pipe_fd_[1]->getHandle(); 
}

}