/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 11:42:42
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <functional>
#include <string>
#include <map>

typedef enum {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT
} HttpMethod;

typedef std::map<std::string, std::string> queryMap;

typedef std::function<bool(const queryMap&, const std::string& body, std::string& response)> httpHandler;

class IHttpServer {
public:
    IHttpServer() = default;
    ~IHttpServer() = default;

    static IHttpServer* instance();
    virtual bool start(uint16_t port = 80) = 0;
    virtual bool stop() = 0;
    virtual bool restart(uint16_t port = 80) = 0;

    virtual bool registerHandler(HttpMethod method, const char* url, httpHandler&& handler) = 0;

    typedef struct {
        std::string www_root_path;
        std::string default_index_file;
    } Config;

    virtual void config(Config& config) = 0;
};
