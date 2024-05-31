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

void RPCClient::processResponse(infra::Buffer &buffer) {
    rpc_header *header = (rpc_header*)buffer.data();
    std::string result(buffer.data() + sizeof(rpc_header), buffer.size() - sizeof(rpc_header));

    std::unique_lock<std::mutex> lock(cb_mtx_);
    auto it = future_map_.find(header->req_id);
    if (it != future_map_.end()) {
        auto& f = future_map_[header->req_id];
        f->set_value(req_result{ req_success, result });
        future_map_.erase(it);
    }
}
