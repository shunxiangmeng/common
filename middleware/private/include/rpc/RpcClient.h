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

template <typename T> inline T get_result(std::string result) {
    msgpack_codec codec;
    auto tp = codec.unpack<std::tuple<int, T>>(result.data(), result.size());
    return std::get<1>(tp);
}

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

    template <size_t TIMEOUT, typename T, typename... Args>
    typename std::enable_if<!std::is_void<T>::value, T>::type
        call(const std::string& rpc_name, Args &&...args) {
        auto future_result = async_call<FUTURE>(rpc_name, std::forward<Args>(args)...);
        auto status = future_result.wait_for(std::chrono::milliseconds(TIMEOUT));
        if (status == std::future_status::timeout || status == std::future_status::deferred) {
            throw std::out_of_range("timeout or deferred");
        }
        return future_result.get().template as<T>();
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

        uint32_t body_size = (uint32_t)ret.size();
        uint32_t message_size = sizeof(rpc_header) + body_size;

        infra::Buffer buffer(message_size);

        uint32_t func_id = infra::MD5Hash32(rpc_name.data());
        rpc_header header = { MAGIC_RPC, body_size, request_type::req_res, fu_id, func_id};;

        buffer.putData((char*)&header, sizeof(header));
        buffer.putData((char*)ret.release(), (int32_t)ret.size());

        if (priv_client_->sendRpcData(buffer) < 0) {
            std::unique_lock<std::mutex> lock(cb_mtx_);
            auto& f = future_map_[fu_id];
            f->set_value(req_result{ req_send_failed, "send failed" });
            future_map_.erase(fu_id);
        }

        return future_result<req_result>{fu_id, std::move(future)};
    }

private:
    friend class PrivClient;
    RPCClient(PrivClient *client);
    ~RPCClient();

    void processResponse(infra::Buffer &buffer);

private:
    PrivClient *const priv_client_;
    uint32_t sequence_ = 1;
    std::mutex cb_mtx_;
    std::unordered_map<std::uint32_t, std::shared_ptr<std::promise<req_result>>> future_map_;

};