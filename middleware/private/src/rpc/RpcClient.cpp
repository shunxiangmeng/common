/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RpcClient.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-30 13:58:18
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "private/include/rpc/RpcClient.h"
#include "../client/PrivClient.h"

RPCClient::RPCClient(PrivClient* client) : priv_client_(client) {
}

RPCClient::~RPCClient() {
}

void RPCClient::processResponse(uint32_t sequence) {
    std::unique_lock<std::mutex> lock(cb_mtx_);
    auto& f = future_map_[sequence];
    f->set_value(req_result{ "ok"});
    future_map_.erase(sequence);
}
