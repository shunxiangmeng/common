/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RpcServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-29 18:49:21
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "private/include/rpc/RpcServer.h"
#include "infra/include/Logger.h"

RPCServer::RPCServer() {
}

RPCServer::~RPCServer() {
}

route_result_t RPCServer::route(infra::Buffer &buffer, std::weak_ptr<infra::TcpSocket> sock) {
    rpc_header* header = (rpc_header*)buffer.data();
    route_result_t ret = router_.route(header->func_id, buffer, sock);

    rpc_header res_header{MAGIC_RPC, (uint32_t)ret.result.size(), request_type::req_res, header->req_id};
    
    buffer_.setSize(0);
    buffer_.ensureCapacity(sizeof(rpc_header) + (int32_t)ret.result.size());
    buffer_.putData((char*)&res_header, sizeof(rpc_header));
    buffer_.putData((char*)ret.result.data(), (int32_t)ret.result.size());

    auto strong_ptr = sock.lock();
    strong_ptr->send(buffer_.data(), buffer_.size());

    return ret;
}