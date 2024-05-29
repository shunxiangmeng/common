/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-27 09:59:19
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <future>
#include <deque>
#include <mutex>
#include <functional>
#include <map>
#include <unordered_map>
#include "../Defs.h"
#include "private/include/MsgpackCodec.h"
#include "private/include/IPrivClient.h"
#include "private/include/Defs.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"
#include "infra/include/Buffer.h"
#include "infra/include/MD5.h"
#include "jsoncpp/include/json.h"

enum class CallModel { future, callback };
const constexpr auto FUTURE = CallModel::future;
const constexpr size_t DEFAULT_TIMEOUT = 1000; // milliseconds

inline bool has_error(std::string result) {
    if (result.empty()) {
        return true;
    }

    //rpc_service::msgpack_codec codec;
    //auto tp = codec.unpack<std::tuple<int>>(result.data(), result.size());

    //return std::get<0>(tp) != 0;
    return 0;
}

inline std::string get_error_msg(std::string result) {
    //rpc_service::msgpack_codec codec;
    //auto tp = codec.unpack<std::tuple<int, std::string>>(result.data(), result.size());
    //return std::get<1>(tp);
    return result;
}

class req_result {
public:
    req_result() = default;
    req_result(std::string data) : data_(data) {}
    
    template <typename T> T as() {
        if (has_error(data_)) {
            std::string err_msg = data_.empty() ? data_ : get_error_msg(data_);
            throw std::logic_error(err_msg);
        }
        return get_result<T>(data_);
    }

    void as() {
        if (has_error(data_)) {
            std::string err_msg = data_.empty() ? data_ : get_error_msg(data_);
            throw std::logic_error(err_msg);
        }
    }

    bool success() const { return !has_error(data_); }

private:
    std::string data_;
};

template <typename T> 
struct future_result {
    uint64_t id;
    std::future<T> future;
    template <class Rep, class Per>
    std::future_status wait_for(const std::chrono::duration<Rep, Per> &rel_time) {
        return future.wait_for(rel_time);
    }

    T get() { return future.get(); }
};

class PrivClient : public IPrivClient, public infra::SocketHandler {
public:
    PrivClient();
    virtual ~PrivClient();

    virtual bool connect(const char* server_ip, uint16_t server_port) override;

    virtual bool testSyncCall() override;

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
            fu_id = sequence_;
            future_map_.emplace(fu_id, std::move(p));
        }

        msgpack_codec codec;
        auto ret = codec.pack_args(std::forward<Args>(args)...);

        uint32_t message_size = sizeof(rpc_header) + ret.size();
        infra::Buffer buffer(message_size);

        //write(fu_id, request_type::req_res, std::move(ret), MD5::MD5Hash32(rpc_name.data()));
        uint32_t func_id = infra::MD5Hash32(rpc_name.data());
        rpc_header header = { "***", request_type::req_res, message_size, 0, func_id};;

        buffer.putData((char*)&header, sizeof(header));
        buffer.putData((char*)ret.release(), ret.size());

        sendRpcData(buffer);

        return future_result<req_result>{fu_id, std::move(future)};
    }

private:
    /**
     * @brief scoket数据输入回调
     * @param fd
     * @return
     */
    virtual int32_t onRead(int32_t fd) override;
    /**
     * @brief socket异常回调
     * @param fd
     * @return
     */
    virtual int32_t onException(int32_t fd) override;

    void sendKeepAlive();

    std::shared_ptr<Message> parse();
    void process(std::shared_ptr<Message> &request);

    int32_t sendRequest(Json::Value &body);

    int32_t sendRpcData(infra::Buffer &buffer);

    infra::Buffer makeRequest(Json::Value &body);

private:
    std::string server_ip_;
    uint16_t server_port_;
    std::shared_ptr<infra::TcpSocket>   sock_;

    infra::Buffer buffer_;

    uint32_t fu_id_ = 0;
    std::mutex cb_mtx_;
    std::unordered_map<std::uint32_t, std::shared_ptr<std::promise<req_result>>> future_map_;

    std::mutex send_mutex_;
    std::mutex sequence_mutex_;
    uint32_t sequence_ = 0;
    uint32_t session_id_ = 0;

};