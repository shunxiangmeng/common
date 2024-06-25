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
    bool deviceLoginLDB();
private:
    int getEDServIP(char *ip, int *port);
    infra::Buffer makeHdRequest(const char* cmd, Json::Value &body, const char* serial);
    int32_t sendData(infra::Buffer& buffer);

    std::shared_ptr<UlucuAsyncCallResult> asyncRequest(const char* cmd, Json::Value &body);
    CallResult syncRequest(const char* cmd, Json::Value &body);

    void onSocketRead(infra::Buffer& buffer);

private:
    std::shared_ptr<infra::TcpIO> sock_;
    std::string device_sn_;

    std::recursive_mutex future_map_mutex_;
    std::unordered_map<std::string, std::shared_ptr<std::promise<ulucu::CallResult>>> future_map_;

    infra::Buffer recv_buffer_;
};

}