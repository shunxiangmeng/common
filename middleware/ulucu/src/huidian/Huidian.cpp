/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  huidian.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-23 15:35:23
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "huidian.h"
#include "infra/include/Logger.h"
#include "infra/include/timestamp.h"

namespace ulucu {
Huidian::Huidian() {
}

Huidian::~Huidian() {
}

bool Huidian::init(const char* sn) {
    device_sn_ = sn;
    // wait for network
    deviceLoginLDB();
    return true;
}

int Huidian::getEDServIP(char *ip, int *port) {
    char buff[64] = {0};
    FILE *fp = fopen (HD_EDServIP_FILE, "r");
    if (fp == NULL) {
	    return -1;
    }
    int ret = fread (buff, 1, sizeof(buff), fp);
    fclose (fp);
    if (ret <= 0 || strlen (buff) <= 0) {
	    return -1;
    }
    sscanf(buff, "%[^:]:%d", ip, port);
    return 0;
}

bool Huidian::deviceLoginLDB() {
    char server_ip[32] = {0};
    int server_port = 0;
    //if (getEDServIP(server_ip, &server_port) < 0) {
    //    errorf("get EDServer error\n");
    //    return false;
    //}

    sock_ = std::make_shared<infra::TcpIO>();
    if (!sock_->connect(HD_BLSERVER_NAME, HD_BLSERVER_PORT, false)) {
        errorf("tcp connect to %s:%d error\n", server_ip, server_port);
        return false;
    }
    tracef("connect %s:%d succeed\n", server_ip, server_port);
    sock_->setNoblocked(true);
    sock_->setCallback([this](infra::Buffer& buffer) {
        return onSocketRead(buffer);
    }, [this](int32_t fd) {
        return 0;
    });

    Json::Value data;
    data["sn"] = device_sn_;
    data["protocol_version"] = HD_PROTOCOL_VER;
    CallResult result = syncRequest("device_login_ldb_req", data);
    if (result.error_code != UlucuErrorCode::SUCCESS) {
        return false;
    }

    return true;
}

void Huidian::onSocketRead(infra::Buffer& buffer) {
    infof("onSocketRead+++\n");
    tracef("%s", buffer.data());
}

infra::Buffer Huidian::makeHdRequest(const char* cmd, Json::Value &body, const char* serial) {
    Json::Value root;
    root["serial"] = device_sn_ + serial;
    root["cmd"] = cmd;
    root["data"] = body;

    Json::FastWriter writer;
    std::string str = writer.write(root);
    str += "\r\n\r\n";
    infra::Buffer buffer(str.size() + 1);
    memcpy((char*)buffer.data(), str.c_str(), str.size());
    buffer.resize(str.size());
    buffer.data()[str.size()] = '\0';
    return buffer;
}

int32_t Huidian::sendData(infra::Buffer& buffer) {
    if (!sock_ || !sock_->isConnected()) {
        return -1;
    }
    return sock_->send((const char*)buffer.data(), buffer.size());
}

std::shared_ptr<UlucuAsyncCallResult> Huidian::asyncRequest(const char* cmd, Json::Value &body) {
    auto promise = std::make_shared<std::promise<ulucu::CallResult>>();
    std::future<CallResult> future = promise->get_future();

    std::string serial = std::to_string(infra::getCurrentTimeUs());
    {
        std::lock_guard<std::recursive_mutex> guard(future_map_mutex_);
        future_map_.emplace(serial, std::move(promise));
    }

    infra::Buffer buffer = makeHdRequest(cmd, body, serial.data());
    tracef("send:%s", buffer.data());
    int32_t send_len = sock_->send((const char*)buffer.data(), buffer.size());
    if (send_len < 0) {
        ulucu::CallResult result;
        result.error_code = UlucuErrorCode::SENDFAILED;
        promise->set_value(std::move(result));
    }
    std::shared_ptr<UlucuAsyncCallResult> async_call_result = std::make_shared<UlucuAsyncCallResult>();
    async_call_result->first = serial;
    async_call_result->second = std::move(future);
    return async_call_result;
}

CallResult Huidian::syncRequest(const char* cmd, Json::Value &body) {
    auto releaseCall = [this](std::string serial) {
        std::lock_guard<std::recursive_mutex> guard(future_map_mutex_);
        auto it = future_map_.find(serial);
        if (it != future_map_.end()) {
            future_map_.erase(it);
        }
    };

    auto async_call_result = asyncRequest(cmd, body);
    auto wait_result = async_call_result->second.wait_for(std::chrono::milliseconds(2000));
    if (wait_result == std::future_status::timeout) {
        releaseCall(async_call_result->first);
        CallResult result;
        result.error_code = UlucuErrorCode::TIMEOUT;
        return result;
    }
    CallResult result = async_call_result->second.get();
    releaseCall(async_call_result->first);
    return result;
}

}