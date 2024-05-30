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
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"
#include "infra/include/Buffer.h"
#include "infra/include/MD5.h"
#include "jsoncpp/include/json.h"

class PrivClient : public IPrivClient, public infra::SocketHandler {
public:
    PrivClient();
    virtual ~PrivClient();

    virtual bool connect(const char* server_ip, uint16_t server_port) override;

    virtual bool testSyncCall() override;

    virtual RPCClient& rpcClient() override {
        return rpc_client_;
    };

    int32_t sendRpcData(infra::Buffer& buffer);

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

    infra::Buffer makeRequest(Json::Value &body);

private:
    std::string server_ip_;
    uint16_t server_port_;
    std::shared_ptr<infra::TcpSocket> sock_;

    infra::Buffer buffer_;

    std::mutex send_mutex_;
    std::mutex sequence_mutex_;
    uint32_t sequence_ = 0;
    uint32_t session_id_ = 0;

    RPCClient rpc_client_;

};