/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  httpServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 12:57:31
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "http/include/IHttpServer.h"
#include "infra/include/network/AcceptSocketV2.h"
#include "ConnectionManager.h"
#include "HttpHandler.h"

class HttpServer : public IHttpServer {
    friend class IHttpServer;
    HttpServer();
    ~HttpServer();
public:
    virtual void config(Config& config) override;
    virtual bool start(uint16_t port = 80) override;
    virtual bool stop() override;
    virtual bool restart(uint16_t port = 80) override;

    virtual bool registerHandler(HttpMethod method, const char* url, httpHandler&& handler) override;

private:
    uint16_t              port_;
    std::shared_ptr<infra::AcceptSocketV2> acceptor_;

    ConnectionManager connection_manager_;
    HttpHandler handler_;
};