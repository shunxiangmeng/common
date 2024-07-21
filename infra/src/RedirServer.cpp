/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RedirServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-21 22:23:45
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <assert.h>
#include <thread>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "infra/include/RedirServer.h"
#include "infra/include/Logger.h"

namespace infra {

RedirServer* RedirServer::instance(const std::string& path) {
    static RedirServer s_instance(path);
    return &s_instance;
}

RedirServer::RedirServer(const std::string& path) : path_(path) {
    assert(!path_.empty());
}

RedirServer::~RedirServer() {
}

struct RedirInfo {
    long type;
    unsigned int flag;
    char inpath[64];
    char outpath[64];
    char errpath[64];
    char res[64];
};

enum RedirFlag {
    redirStdIn = 1,    ///< 切换输入
    redirStdOut = 2,   ///< 切换输出
    redirStdError = 4, ///< 切换错误输出
    redirFile = 8	   ///< 重定向到文件
};

bool RedirServer::start() {
    int fd_out_bak = dup(STDOUT_FILENO);
    int fd_in_bak = dup(STDIN_FILENO);
    assert(fd_out_bak >= 0 && fd_in_bak >= 0);
    if (path_.empty()|| (creat(path_.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1)) {
        errorf("%s[%d]:error!\n", __FILE__, __LINE__);
        return false;
    }

    std::thread([this, &fd_out_bak, &fd_in_bak] () {
        infof("start redir server\n");
        RedirInfo iopath;
        key_t msg_key = -1;
        running_ = true;
        while (running_) {
            msg_key = ftok(path_.c_str(), 1);
            if (msg_key != -1) {
                int msgid = msgget(msg_key, IPC_CREAT | 0666);
                if (msgid != -1) {
                    msgrcv(msgid, &iopath, sizeof(iopath) - sizeof(long), 1, MSG_NOERROR);
                    if ((iopath.flag & redirStdIn) != 0) {
                        int fd_in = open(iopath.inpath, O_RDONLY | O_NONBLOCK);
                        if (fd_in >= 0) {
                            printf("redir stdin......\n");
                            warnf("redir stdin......\n");
                            dup2(fd_in, STDIN_FILENO);
                            close(fd_in);
                        }
                    } else {
                        dup2(fd_in_bak, STDIN_FILENO);
                    }

                    if ((iopath.flag & redirStdOut) != 0) {
                        int fd_out = -1;
                        if ((iopath.flag & redirFile) != 0) {
                            fd_out = open(iopath.outpath, O_RDWR | O_CREAT | O_APPEND | O_NONBLOCK, 0666);
                        } else {
                            fd_out = open(iopath.outpath, O_WRONLY | O_NONBLOCK);
                        }
                        if (fd_out >= 0) {
                            printf("redir stdout......\n");
                            warnf("redir stdout......\n");
                            dup2(fd_out, STDOUT_FILENO);
                            close(fd_out);
                        }
                    } else {
                        dup2(fd_out_bak, STDOUT_FILENO);
                    }
                }
            }
            sleep(1);
        }
    }).detach();
    return true;
}

bool RedirServer::stop() {
    running_ = false;
    return true;
}

}
