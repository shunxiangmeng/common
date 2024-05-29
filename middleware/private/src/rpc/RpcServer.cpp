/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RpcServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-29 18:49:21
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "private/include/RpcServer.h"

RPCServer::RPCServer() {
}

RPCServer::~RPCServer() {
}

route_result_t RPCServer::route(infra::Buffer &buffer) {
    rpc_header* header = (rpc_header*)buffer.data();
    route_result_t ret = router_.route(header->func_id, buffer);
    return ret;
}