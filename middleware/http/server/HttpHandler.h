/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpHandler.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-03 00:05:48
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <map>
#include "Request.h"
#include "Reply.h"
#include "http/include/IHttpServer.h"

class HttpHandler {
public:
    HttpHandler() = default;
    ~HttpHandler() = default;

    void handleRequest(Request& request, Reply& reply);
    bool registerHandler(IHttpServer::HttpMethod method, const char* url, httpHandler&& handler);
    bool unRegisterHandler(IHttpServer::HttpMethod method, const char* url, httpHandler&& handler);

    void config(IHttpServer::Config& config);

private:
    bool isApiRequest(Request& request);
    void doApiRequest(Request& request, Reply& reply);
    std::string getFileExtenssion(std::string& filename);

private:
    std::string www_root_path_;
    std::string default_index_file_;
    std::map<std::string, httpHandler> router_map_;
    std::mutex router_map_mutex_;
};
