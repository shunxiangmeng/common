/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Main.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-21 21:43:50
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <string>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BRONCO_PATH "/tmp/broncoRedir"
#define ROBCAT_PATH "/tmp/robcatRedir"

static std::string s_pid_path;
static std::string s_sem_path;

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

//< 阻塞
bool waitForever() {
    sem_t sem;
    memset(&sem, 0, sizeof(sem_t));
    int iRet = sem_init(&sem, 0, 0);
    if (iRet < 0) {
        printf("sem_init failed\n");
        return false;
    }

    while ((iRet = sem_wait(&sem)) != 0 && errno == EINTR) {
        ;
    }
    return true;
}

//< 重定向
bool redirToTerminal(const std::string &path, unsigned int flag) {
    key_t msg_key = ftok(path.c_str(), 1);
    if (msg_key < 0) {
        printf("ftok %s error: %s\n", path.data(), strerror(errno));
        return false;
    }
    RedirInfo redirinfo;
    memset(&redirinfo, 0, sizeof(redirinfo));
    redirinfo.type = 1;
    snprintf(redirinfo.outpath, sizeof(redirinfo.outpath), "%s", ttyname(STDOUT_FILENO));
    snprintf(redirinfo.inpath, sizeof(redirinfo.inpath), "%s", ttyname(STDIN_FILENO));
    redirinfo.flag = flag;
    int msgid = msgget(msg_key, IPC_CREAT | 0666);
    if (msgid < 0) {
        printf("msgget error\n");
        return false;
    }
    if (-1 == msgsnd(msgid, &redirinfo, sizeof(redirinfo) - sizeof(long), 0)) {
        printf("%s [%s] failed!\n", __FUNCTION__, path.c_str());
        exit(127);
    }
    printf("%s [%s] successfully!\n", __FUNCTION__, path.c_str());
    if (flag > 0) {
        waitForever();
    }
    return true;
}

/// 关闭重定向
static void closeRedir() {
    redirToTerminal(s_sem_path, 0);
}

//异常信号处理
void onExit(int signal) {
    printf("SIG : %d\n", signal);
    //挂起、ctrl+C、网络断开等异常情况时，重定向复位
    if (signal == SIGHUP || signal == SIGINT || signal == SIGQUIT || signal == SIGPIPE || signal == SIGSEGV) {
        printf("tty or redirClient is abort!\n");
        closeRedir();
    }
    //被兄弟进程杀死或者强制终结则打印告知用户并安全退出
    else if (signal == SIGKILL || signal == SIGTERM) {
        printf("Another redirClient start up! I will stop myself!\n");
    }

    //终端属性复位
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tty.c_lflag |= ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    exit(0);
}

static void detectProc() {
    int fd = open(s_pid_path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        printf("open file %s error\n", s_pid_path.c_str());
        return;
    }

    //< 尝试锁定文件
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    int ret = fcntl(fd, F_SETLK, &fl); //< 非阻塞
    if (ret == -1) {
        //< 锁定失败
        char buffer[16] = {0};
        read(fd, buffer, 16);
        printf("anther redirClient is running! PID:%s\n", buffer);
        //由用户判断是否强制启动
        printf("Do you want to start up and kill anther redirClient?\n");
        printf("(y or n)?");
        char ch = 'n';
        scanf("%c", &ch);
        if (ch == 'y') {
            char killer[128];
            sprintf(killer, "kill -9 %s", buffer);
            system(killer);
            sleep(1);
            printf("I killed it already!\n");
        } else if (ch == 'n') {
            close(fd);
            exit(0);
        }
        //< 重新获取锁
        ret = fcntl(fd, F_SETLKW, &fl); //< 阻塞
        if (ret == -1) {
            close(fd);
            exit(0);
        }
    }
    std::string pid = std::to_string(getpid());
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    write(fd, pid.c_str(), pid.length());
}

int main(int argc, char *argv[]) {
    if (argc <= 1 || argv[1] == NULL) {
        s_pid_path = BRONCO_PATH;
        s_sem_path = BRONCO_PATH;
        s_pid_path += ".pid";
    } else {
        if (strcasecmp(argv[1], "bronco") == 0) {
            s_pid_path = BRONCO_PATH;
            s_sem_path = BRONCO_PATH;
            s_pid_path += ".pid";
        } else if (strcasecmp(argv[1], "UP") == 0) {
            s_pid_path = ROBCAT_PATH;
            s_sem_path = ROBCAT_PATH;
            s_pid_path += ".pid";
        } else {
            printf("params is error\n");
            return 0;
        }
    }

    //< 检查同名进程
    detectProc();

    //注册异常处理函数，防止异常情况下该工具造成设备异常重启
    struct sigaction myAction;
    myAction.sa_handler = onExit;
    sigemptyset(&myAction.sa_mask);
    myAction.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGHUP, &myAction, NULL);  // 终端挂起
    sigaction(SIGINT, &myAction, NULL);  // Ctrl+C
    sigaction(SIGQUIT, &myAction, NULL); // 键盘退出
    sigaction(SIGPIPE, &myAction, NULL); // 网络异常
    sigaction(SIGKILL, &myAction, NULL); // 被kill -9
    sigaction(SIGSEGV, &myAction, NULL); // 内存访问异常，可能是所在终端被关闭
    sigaction(SIGTERM, &myAction, NULL); // 被终结

    redirToTerminal(s_sem_path, redirStdIn | redirStdOut);
    return 0;
}
