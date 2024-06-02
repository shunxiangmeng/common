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

future_result<req_result> RPCClient::doAsyncCall(const std::string &rpc_name, msgpack::sbuffer& sbuffer) {
    auto p = std::make_shared<std::promise<req_result>>();
    std::future<req_result> future = p->get_future();

    uint32_t fu_id = 0;
    {
        std::unique_lock<std::mutex> lock(cb_mtx_);
        fu_id = sequence_++;
        future_map_.emplace(fu_id, std::move(p));
    }

    uint32_t body_size = (uint32_t)sbuffer.size();
    uint32_t message_size = sizeof(rpc_header) + body_size;

    infra::Buffer buffer(message_size);

    uint32_t func_id = infra::MD5Hash32(rpc_name.data());
    rpc_header header = { MAGIC_RPC, body_size, request_type::req_res, fu_id, func_id};;

    buffer.putData((char*)&header, sizeof(header));
    buffer.putData((char*)sbuffer.release(), (int32_t)body_size);

    if (priv_client_->sendRpcData(buffer) < 0) {
        std::unique_lock<std::mutex> lock(cb_mtx_);
        auto& f = future_map_[fu_id];
        f->set_value(req_result{ req_send_failed, "send failed" });
        future_map_.erase(fu_id);
    }
    return future_result<req_result>{fu_id, std::move(future)};
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
