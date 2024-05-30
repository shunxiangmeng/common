/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Router.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-28 20:42:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <tuple>
#include "MetaUtil.h"
#include "MsgpackCodec.h"
#include "Defs.h"
#include "infra/include/MD5.h"
#include "infra/include/Buffer.h"
#include "infra/include/network/TcpSocket.h"

enum class router_error { ok, no_such_function, has_exception, unkonw };

struct route_result_t {
    router_error ec = router_error::unkonw;
    std::string result;
};

template <typename Tuple, bool is_pub> 
class helper_t {
public:
    helper_t(Tuple& tp) : tp_(tp) {}
    void operator()() {}
private:
    Tuple& tp_;
};

template <typename Tuple> 
class helper_t<Tuple, true> {
public:
    helper_t(Tuple& tp) : tp_(tp) {}
    void operator()() {
        auto& arg = std::get<std::tuple_size<Tuple>::value - 1>(tp_);
        msgpack_codec codec;
        arg = codec.unpack<std::string>(arg.data(), arg.size());
    }
private:
    Tuple& tp_;
};

class Router {
public:
    Router() = default;

    template <bool is_pub = false, typename Function>
    void register_handler(std::string const &name, Function f, bool pub = false) {
        uint32_t key = infra::MD5Hash32(name.data());
        key2func_name_.emplace(key, name);
        return register_nonmember_func<is_pub>(key, std::move(f));
    }

    void remove_handler(std::string const &name) {
        uint32_t key = infra::MD5Hash32(name.data());
        this->map_invokers_.erase(key);
        key2func_name_.erase(key);
    }

    std::string get_name_by_key(uint32_t key) {
        auto it = key2func_name_.find(key);
        if (it != key2func_name_.end()) {
            return it->second;
        }
        return std::to_string(key);
    }

    route_result_t route(uint32_t key, infra::Buffer &data, std::weak_ptr<infra::TcpSocket> sock) {
        route_result_t route_result{};
        std::string result;
        try {
            msgpack_codec codec;
            auto it = map_invokers_.find(key);
            if (it == map_invokers_.end()) {
                result = codec.pack_args_str(result_code::FAIL, "unknown function: " + get_name_by_key(key));
                route_result.ec = router_error::no_such_function;
            } else {
                it->second(sock, data, result);
                route_result.ec = router_error::ok;
            }
        } catch (const std::exception &ex) {
            msgpack_codec codec;
            result = codec.pack_args_str(result_code::FAIL, std::string("exception occur when call").append(ex.what()));
            route_result.ec = router_error::has_exception;
        } catch (...) {
            msgpack_codec codec;
            result = codec.pack_args_str(result_code::FAIL, std::string("unknown exception occur when call ").append(get_name_by_key(key)));
            route_result.ec = router_error::no_such_function;
        }

        route_result.result = std::move(result);
        return route_result;
    }

private:
    Router(const Router&) = delete;
    Router(Router&&) = delete;

    template <typename F, size_t... I, typename... Args>
    static typename std::result_of<F(std::weak_ptr<infra::TcpSocket>, Args...)>::type
        call_helper(const F& f, const std::index_sequence<I...>&, std::tuple<Args...> tup, std::weak_ptr<infra::TcpSocket> sock) {
        return f(sock, std::move(std::get<I>(tup))...);
    }

    template <typename F, typename... Args>
    static typename std::enable_if<std::is_void<typename std::result_of<F(std::weak_ptr<infra::TcpSocket>, Args...)>::type>::value>::type
        call(const F& f, std::weak_ptr<infra::TcpSocket> sock, std::string& result, std::tuple<Args...> tp) {
        call_helper(f, std::make_index_sequence<sizeof...(Args)>{}, std::move(tp), sock);
        result = msgpack_codec::pack_args_str(result_code::OK);
    }

    template <typename F, typename... Args>
    static typename std::enable_if<!std::is_void<typename std::result_of<F(std::weak_ptr<infra::TcpSocket>, Args...)>::type>::value>::type
        call(const F& f, std::weak_ptr<infra::TcpSocket> sock, std::string& result, std::tuple<Args...> tp) {
        auto r = call_helper(f, std::make_index_sequence<sizeof...(Args)>{}, std::move(tp), sock);
        msgpack_codec codec;
        result = msgpack_codec::pack_args_str(result_code::OK, r);
    }

    template <bool is_pub, typename Function>
    void register_nonmember_func(uint32_t key, Function f) {
        this->map_invokers_[key] = [f](std::weak_ptr<infra::TcpSocket> sock, infra::Buffer &data, std::string& result) {
            using args_tuple = typename function_traits<Function>::bare_tuple_type;
            msgpack_codec codec;
            try {
                auto tp = codec.unpack<args_tuple>(data.data() + sizeof(rpc_header), data.size() - sizeof(rpc_header));
                helper_t<args_tuple, is_pub>{tp}();
                call(f, sock, result, std::move(tp));
            }
            catch (std::invalid_argument& e) {
                result = codec.pack_args_str(result_code::FAIL, e.what());
            }
            catch (const std::exception& e) {
                result = codec.pack_args_str(result_code::FAIL, e.what());
            }
        };
    }


    template <typename F, typename Self, size_t... Indexes, typename... Args>
    static typename std::result_of<F(Self, std::weak_ptr<infra::TcpSocket>, Args...)>::type
        call_member_helper(const F &f, Self *self, 
                            const std::index_sequence<Indexes...> &,
                            std::tuple<Args...> tup,
                            std::weak_ptr<infra::TcpSocket> sock = std::shared_ptr<infra::TcpSocket>{nullptr}) {
        return (*self.*f)(sock, std::move(std::get<Indexes>(tup))...);
    }

    template <typename F, typename Self, typename... Args>
    static typename std::enable_if<std::is_void<typename std::result_of<F(Self, std::weak_ptr<infra::TcpSocket>, Args...)>::type>::value>::type
    call_member(const F &f, Self *self, std::weak_ptr<infra::TcpSocket> sock, std::string &result, std::tuple<Args...> tp) {
        call_member_helper(f, self, typename std::make_index_sequence<sizeof...(Args)>{}, std::move(tp), sock);
        result = msgpack_codec::pack_args_str(result_code::OK);
    }

    template <typename F, typename Self, typename... Args>
    static typename std::enable_if<!std::is_void<typename std::result_of<F(Self, std::weak_ptr<infra::TcpSocket>, Args...)>::type>::value>::type
    call_member(const F &f, Self *self, std::weak_ptr<infra::TcpSocket> sock, std::string &result, std::tuple<Args...> tp) {
        auto r = call_member_helper(f, self, typename std::make_index_sequence<sizeof...(Args)>{}, std::move(tp), sock);
        result = msgpack_codec::pack_args_str(result_code::OK, r);
    }

    template <bool is_pub, typename Function, typename Self>
    void register_member_func(uint32_t key, const Function &f, Self *self) {
        this->map_invokers_[key] = [f, self](std::weak_ptr<infra::TcpSocket> sock, infra::Buffer &data, std::string &result) {
            using args_tuple = typename function_traits<Function>::bare_tuple_type;
            msgpack_codec codec;
            try {
                auto tp = codec.unpack<args_tuple>(data.data(), data.size());
                helper_t<args_tuple, is_pub>{tp}();
                call_member(f, self, sock, result, std::move(tp));
            } catch (std::invalid_argument &e) {
                result = codec.pack_args_str(result_code::FAIL, e.what());
            } catch (const std::exception &e) {
                result = codec.pack_args_str(result_code::FAIL, e.what());
            }
        };
    }

private:
    std::unordered_map<uint32_t, std::function<void(std::weak_ptr<infra::TcpSocket> &, infra::Buffer &, std::string &)>> map_invokers_;
    std::unordered_map<uint32_t, std::string> key2func_name_;
};