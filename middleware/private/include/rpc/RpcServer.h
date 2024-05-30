/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RpcServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-28 20:38:36
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "Router.h"
#include "infra/include/Buffer.h"

class PrivSessionBase;
class RPCServer {
public:
    template <bool is_pub = false, typename Function>
    void register_handler(std::string const &name, const Function &f) {
        router_.register_handler<is_pub>(name, f);
    }

    template <bool is_pub = false, typename Function, typename Self>
    void register_handler(std::string const &name, const Function &f, Self *self) {
        router_.register_handler<is_pub>(name, f, self);
    }

    route_result_t route(infra::Buffer &buffer, std::weak_ptr<infra::TcpSocket> sock);

private:
    friend class PrivServer;
    RPCServer();
    ~RPCServer();

private:
    Router router_;
    infra::Buffer buffer_;
};