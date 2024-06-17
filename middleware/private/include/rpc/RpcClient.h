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
#include "infra/include/Optional.h"
#include "infra/include/Logger.h"

class PrivClient;
class RPCClient {
public:
    template <size_t TIMEOUT = DEFAULT_TIMEOUT, typename T = void, typename... Args>
    typename std::enable_if<std::is_void<T>::value>::type 
    call(const std::string &rpc_name, Args &&...args) {
        try {
            auto future_result = async_call<FUTURE>(rpc_name, std::forward<Args>(args)...);
            auto status = future_result.wait_for(std::chrono::milliseconds(TIMEOUT));
            if (status == std::future_status::timeout || status == std::future_status::deferred) {
                //throw std::out_of_range("timeout or deferred");
                errorf("rpc call %s timeout\n", rpc_name.data());
            }
            future_result.get().as();
        } catch (std::exception &ex) {
            errorf("rpc call exception %s\n", ex.what());
        }
    }

    template <typename T, typename... Args>
    typename std::enable_if<!std::is_void<T>::value, infra::optional<T>>::type
    call(const std::string &rpc_name, Args &&...args) {
        return call<DEFAULT_TIMEOUT, T>(rpc_name, std::forward<Args>(args)...);
    }

    template <size_t TIMEOUT, typename T, typename... Args>
    typename std::enable_if<!std::is_void<T>::value, infra::optional<T>>::type
        call(const std::string& rpc_name, Args &&...args) {
        infra::optional<T> result;
        try {
            auto future_result = async_call<FUTURE>(rpc_name, std::forward<Args>(args)...);
            auto status = future_result.wait_for(std::chrono::milliseconds(TIMEOUT));
            if (status == std::future_status::timeout || status == std::future_status::deferred) {
                //throw std::out_of_range("timeout or deferred");
                errorf("rpc call %s timeout\n", rpc_name.data());
                return result;
            }
            T t = future_result.get().template as<T>();
            result = t;
            return result;
        } catch (std::exception &ex) {
            errorf("rpc call %s exception: %s\n", rpc_name.data(), ex.what());
            return result;
        }
    }

    template <CallModel model, typename... Args>
    future_result<req_result> async_call(const std::string &rpc_name, Args &&...args) {
        msgpack_codec codec;
        auto ret = codec.pack_args(std::forward<Args>(args)...);
        return doAsyncCall(rpc_name, ret);
    }

private:
    friend class PrivClient;
    RPCClient(PrivClient *client);
    ~RPCClient();

    void processResponse(const char* buffer, int32_t size);
    future_result<req_result> doAsyncCall(const std::string &rpc_name, msgpack::sbuffer& sbuffer);

private:
    PrivClient *const priv_client_;
    uint32_t sequence_ = 1;
    std::mutex cb_mtx_;
    std::unordered_map<std::uint32_t, std::shared_ptr<std::promise<req_result>>> future_map_;

};