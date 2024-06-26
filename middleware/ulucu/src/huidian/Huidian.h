/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  huidian.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-23 15:35:29
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <mutex>
#include <unordered_map>
#include "infra/include/network/TcpIO.h"
#include "infra/include/Buffer.h"
#include "jsoncpp/include/json.h"
#include "Result.h"
#include "HuidianHandler.h"

namespace ulucu {

#define HD_BLSERVER_NAME "entry.device.serv.ulucu.com"
#define HD_BLSERVER_PORT 29023
#define HD_EDServIP_FILE "EDServIP"
#define HD_PROTOCOL_VER	 "2.1.2"

typedef enum {
    HD_MACH_LOGIN_LDB = 0,
    HD_MACH_LDB_RECV,
    HD_MACH_LDB_ACK,
    HD_MACH_LOGIN_MGR,
    HD_MACH_MGR_RECV,
    HD_MACH_LOGIN_MGR_ACK,
    HD_MACH_MGR_CMD,
    HD_MACH_MGR_BEAT,
    HD_MACH_IDLE_CHECK,
} hd_status_machine;

class Huidian {
public:
    Huidian();
    ~Huidian();
    bool init(const char* sn);
    bool device_login_ldb();
    bool device_login_mgr();
private:
    int getEDServIP(char *ip, int *port);
    infra::Buffer makeHdRequest(const char* cmd, Json::Value &body, const char* serial);
    int32_t sendData(infra::Buffer& buffer);

    std::shared_ptr<UlucuAsyncCallResult> asyncRequest(const char* cmd, Json::Value &body);
    CallResult syncRequest(const char* cmd, Json::Value &body);

    void onSocketRead(infra::Buffer& buffer);
    void onRequestFromServer(std::string& cmd, std::string& serial, Json::Value& data);
    void responseToServer(std::string& cmd, std::string& serial, int32_t code, std::string &reason, Json::Value &body);
    infra::Buffer makeHdResponse(std::string& cmd, std::string& serial, int32_t code, std::string &reason, Json::Value &body);


    void initHandlerList();
    int32_t call(std::string key, Json::Value& data, Json::Value& result, std::string& reason);

    template <typename Function, typename Self>
    void registerMethodFunc(std::string&& key, const Function& f, Self* self) {
        huidian_handlers_[key] = [f, self](Json::Value& data, Json::Value& result, std::string& reason) {
            return (*self.*f)(data, result, reason);
        };
    }

private:
    std::shared_ptr<infra::TcpIO> sock_;
    std::string device_sn_;

    std::recursive_mutex future_map_mutex_;
    std::unordered_map<std::string, std::shared_ptr<std::promise<ulucu::CallResult>>> future_map_;

    infra::Buffer recv_buffer_;
    char devmgr_server_ip_[32];
    uint16_t devmgr_server_port_;
    std::string token_;
    int32_t login_count_ = 0;
    bool connect_mgr_ = false;

    HuidianHandler handler_;
    std::unordered_map<std::string, std::function<int32_t(Json::Value&, Json::Value&, std::string&)>> huidian_handlers_;
};

}