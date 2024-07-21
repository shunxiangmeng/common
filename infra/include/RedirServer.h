/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RedirServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-21 22:23:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>

namespace infra {

class RedirServer {
    RedirServer(const RedirServer&);
    RedirServer& operator=(const RedirServer&);
public:
    static RedirServer* instance(const std::string& path);
    RedirServer(const std::string& path);
    ~RedirServer();
    bool start();
    bool stop();
private:
    std::string path_;
    bool running_;
};

}