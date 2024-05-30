/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RpcClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-30 10:56:32
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <unordered_map>
#include "Router.h"

class PrivClient;
class RPCClient {
public:
    template <size_t TIMEOUT = DEFAULT_TIMEOUT, typename T = void, typename... Args>
    typename std::enable_if<std::is_void<T>::value>::type 
    call(const std::string &rpc_name, Args &&...args) {
        auto future_result = async_call<FUTURE>(rpc_name, std::forward<Args>(args)...);
        auto status = future_result.wait_for(std::chrono::milliseconds(TIMEOUT));
        if (status == std::future_status::timeout || status == std::future_status::deferred) {
            throw std::out_of_range("timeout or deferred");
        }
        future_result.get().as();
    }

    template <typename T, typename... Args>
    typename std::enable_if<!std::is_void<T>::value, T>::type
    call(const std::string &rpc_name, Args &&...args) {
        return call<DEFAULT_TIMEOUT, T>(rpc_name, std::forward<Args>(args)...);
    }


    template <CallModel model, typename... Args>
    future_result<req_result> async_call(const std::string &rpc_name, Args &&...args) {
        auto p = std::make_shared<std::promise<req_result>>();
        std::future<req_result> future = p->get_future();

        uint32_t fu_id = 0;
        {
            std::unique_lock<std::mutex> lock(cb_mtx_);
            fu_id = sequence_++;
            future_map_.emplace(fu_id, std::move(p));
        }

        msgpack_codec codec;
        auto ret = codec.pack_args(std::forward<Args>(args)...);

        uint32_t message_size = sizeof(rpc_header) + (uint32_t)ret.size();
        infra::Buffer buffer(message_size);

        //write(fu_id, request_type::req_res, std::move(ret), MD5::MD5Hash32(rpc_name.data()));
        uint32_t func_id = infra::MD5Hash32(rpc_name.data());
        rpc_header header = { MAGIC_RPC, message_size, request_type::req_res, 0, func_id};;

        buffer.putData((char*)&header, sizeof(header));
        buffer.putData((char*)ret.release(), (int32_t)ret.size());

        priv_client_->sendRpcData(buffer);

        return future_result<req_result>{fu_id, std::move(future)};
    }

private:
    friend class PrivClient;
    RPCClient(PrivClient *client);
    ~RPCClient();

    void processResponse(infra::Buffer &buffer);

private:
    PrivClient *const priv_client_;
    uint32_t sequence_ = 0;
    std::mutex cb_mtx_;
    std::unordered_map<std::uint32_t, std::shared_ptr<std::promise<req_result>>> future_map_;

};