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
#include <deque>
#include <mutex>
#include <functional>
#include <map>
#include <unordered_map>
#include "private/include/IPrivClient.h"
#include "../Defs.h"
#include "PrivCallResult.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"
#include "infra/include/Buffer.h"
#include "jsoncpp/include/json.h"

template <typename T> 
struct FutureResult {
    uint64_t id;
    std::future<T> future;
    template <class Rep, class Per>
    std::future_status wait_for(const std::chrono::duration<Rep, Per> &rel_time) {
        return future.wait_for(rel_time);
    }
    T get() { return future.get(); }
};

#define REQUEST_TIMEOUT 500

class PrivClient : public IPrivClient, public infra::SocketHandler {
public:
    PrivClient();
    virtual ~PrivClient();

    virtual bool connect(const char* server_ip, uint16_t server_port) override;
    virtual bool startPreview(int32_t channel, int32_t sub_channel, OnFrameProc onframe) override;
    virtual bool stopPreview(OnFrameProc onframe) override;
    virtual bool subscribeEvent(const char* event, EventFunction event_callback) override;
    virtual bool getVideoFormat(std::string &format) override;
    virtual bool setVideoFormat(std::string &format) override;
    virtual bool getVideoConfig(Json::Value &video_config) override;

    virtual bool testSyncCall() override;

    virtual RPCClient& rpcClient() override {
        return rpc_client_;
    };

    int32_t sendRpcData(infra::Buffer& buffer);

private:
    virtual int32_t onRead(int32_t fd) override;
    virtual int32_t onException(int32_t fd) override;
    bool ensureRead(int32_t &readed_size);

    void onDisconnect();

    bool login();

    void onMediaFrame(std::shared_ptr<Message> &message);

    void sendKeepAlive();

    std::shared_ptr<Message> parse();

    void process(const char* buffer, int32_t size);
    void process(std::shared_ptr<Message> &request);
    void onResponse(std::shared_ptr<Message> &message);
    void onRequest(std::shared_ptr<Message>& message);

    void processRpc(const char* buffer, int32_t size);

    int32_t sendRequest(Json::Value &body);

    CallResult syncRequest(Json::Value &body);
    std::shared_ptr<AsyncCallResult> asyncRequest(Json::Value &body);

    infra::Buffer makeRequest(uint32_t sequence, Json::Value &body);

    bool event(std::shared_ptr<Message>& message);

private:
    void initMethodList();
    bool call(std::string key, std::shared_ptr<Message>& message);

    template <typename Function, typename Self>
    void registerMethodFunc(std::string&& key, const Function& f, Self* self) {
        this->map_invokers_[key] = [f, self](std::shared_ptr<Message>& message) {
            return (*self.*f)(message);
        };
    }
private:
    std::string server_ip_;
    uint16_t server_port_;
    std::shared_ptr<infra::TcpSocket> sock_;

    infra::Buffer buffer_;

    std::mutex send_mutex_;
    uint32_t sequence_ = 0;
    uint32_t session_id_ = 0;

    RPCClient rpc_client_;

    std::recursive_mutex future_map_mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<std::promise<CallResult>>> future_map_;

    PrivClientSignal media_signal_;
    int32_t preview_channel_;
    int32_t preview_sub_channel_;

    std::unordered_map<std::string, std::function<bool(std::shared_ptr<Message>&)>> map_invokers_;
    EventFunction event_callback_;
};