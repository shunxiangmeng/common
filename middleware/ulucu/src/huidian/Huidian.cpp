/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  huidian.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-23 15:35:23
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Huidian.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"

namespace ulucu {
Huidian::Huidian() {
}

Huidian::~Huidian() {
    keep_alive_timer_.stop();
}

bool Huidian::init(const char* sn) {
    initHandlerList();
    device_sn_ = sn;
    // wait for network
    if (!device_login_ldb()) {
        return false;
    }
    if (!device_login_mgr()) {
        return false;
    }
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

void Huidian::onSocketRead(infra::Buffer& buffer) {
    //infof("onSocketRead+++\n");
    //std::string tmp(buffer.data(), buffer.size());
    //tracef("size:%d, %s", tmp.size(), tmp.data());
    char* separator_pos = strstr(buffer.data(), "\r\n\r\n");
    if (separator_pos) {

        int32_t used_len = separator_pos - buffer.data() + 4;
        int32_t left_size = buffer.size() - used_len;
        if (left_size) {
            memmove(buffer.data(), buffer.data() + used_len, left_size);
        }
        buffer.resize(left_size);

        Json::Value root = Json::nullValue;
        Json::String err;
        Json::CharReaderBuilder readbuilder;
        std::unique_ptr<Json::CharReader> jsonreader(readbuilder.newCharReader());
        jsonreader->parse(buffer.data(), separator_pos, &root, &err);
        if (root.empty()) {
            return;
        }

        std::string cmd, cmd_suffix;
        int32_t code = -1;
        std::string reason;
        std::string serial;

        if (root.isMember("cmd") && root["cmd"].isString()) {
            cmd = root["cmd"].asString();
            cmd_suffix = cmd.length() > 4 ? cmd.substr(cmd.length() - 4, 4) : "";
        } else {
            errorf("cmd field error\n");
            return;
        }
        if (root.isMember("serial") && root["serial"].isString()) {
            serial = root["serial"].asString();
        }
        if (root.isMember("error") && root["error"].isInt()) {
            code = root["error"].asInt();
        }
        if (root.isMember("reason") && root["reason"].isString()) {
            reason = root["reason"].asString();
        }

        if (cmd_suffix == "_req") {
            // req
            onRequestFromServer(cmd, serial, root["data"]);
        } else {
            // ack
            std::lock_guard<std::recursive_mutex> guard(future_map_mutex_);
            auto it = future_map_.find(serial);
            if (it != future_map_.end()) {
                auto& promise = it->second;
                ulucu::CallResult result;
                result.error_code = UlucuErrorCode::SUCCESS;
                result.code = code;
                result.reason = reason;
                result.root = std::move(root);
                promise->set_value(std::move(result));
            }
        }
    }
}

void Huidian::onRequestFromServer(std::string& cmd, std::string& serial, Json::Value& data) {
    tracef("req cmd:%s\n", cmd.data());
    Json::Value result = Json::objectValue;
    std::string reason;
    int32_t error_code = call(cmd, data, result, reason);
    responseToServer(cmd, serial, error_code, reason, result);
}

void Huidian::responseToServer(std::string& cmd, std::string& serial, int32_t code, std::string &reason, Json::Value &body) {
    infra::Buffer buffer = makeHdResponse(cmd, serial, code, reason, body);
    if (buffer.empty()) {
        errorf("response to server failed\n");
        return;
    }
    tracef("response:%s", buffer.data());
    sendData(buffer);
}

infra::Buffer Huidian::makeHdResponse(std::string& cmd, std::string& serial, int32_t code, std::string &reason, Json::Value &body) {
    Json::Value root;
    root["serial"] = serial;
    root["cmd"] = cmd.substr(0, cmd.length() - 4) + "_ack";
    root["error"] = code;
    root["reason"] = code == 0 ? "ok" : reason;
    root["sn"] = device_sn_;
    root["data"] = body;

    Json::FastWriter writer;
    std::string str = writer.write(root);
    str += "\r\n\r\n";
    infra::Buffer buffer(str.size() + 1);
    if (buffer.empty()) {
        errorf("malloc buffer size:%d failed\n", str.size() + 1);
        return buffer;
    }
    buffer.putData(str.c_str(), str.size());
    buffer.data()[str.size()] = '\0';
    return buffer;
}

infra::Buffer Huidian::makeHdRequest(const char* cmd, Json::Value &body, const char* serial) {
    Json::Value root;
    root["serial"] = serial;
    root["cmd"] = cmd;
    root["data"] = body;

    Json::FastWriter writer;
    std::string str = writer.write(root);
    str += "\r\n\r\n";
    infra::Buffer buffer(str.size() + 1);
    if (buffer.empty()) {
        errorf("malloc buffer size:%d failed\n", str.size() + 1);
        return buffer;
    }
    buffer.putData(str.c_str(), str.size());
    buffer.data()[str.size()] = '\0';
    return buffer;
}

int32_t Huidian::sendData(infra::Buffer& buffer) {
    if (!sock_ || !sock_->isConnected()) {
        errorf("sock is not init\n");
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
        future_map_.emplace(serial, promise);
    }

    infra::Buffer buffer = makeHdRequest(cmd, body, serial.data());
    tracef("send data: %s", buffer.data());
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

bool Huidian::device_login_ldb() {
    char* server_ip = HD_BLSERVER_NAME;
    int server_port = HD_BLSERVER_PORT;
    //if (getEDServIP(server_ip, &server_port) < 0) {
    //    errorf("get EDServer error\n");
    //    return false;
    //}

    sock_ = std::make_shared<infra::TcpIO>();
    if (!sock_->connect(server_ip, server_port, false)) {
        errorf("tcp connect to %s:%d error\n", server_ip, server_port);
        return false;
    }
    tracef("connect %s:%d succeed\n", server_ip, server_port);
    sock_->setNoblocked(true);
    sock_->setCallback([this](infra::Buffer& buffer) {
        onSocketRead(buffer);
    }, [this](int32_t fd) {
    });

    Json::Value data;
    data["sn"] = device_sn_;
    data["protocol_version"] = HD_PROTOCOL_VER;
    CallResult result = syncRequest("device_login_ldb_req", data);
    if (!result.success()) {
        errorf("%s\n", result.root.toStyledString().data());
        errorf("device_login_ldb failed, code:%d, reason:%s\n", result.code, result.reason.data());
        sock_->stop();
        return false;
    } else {
        Json::Value data = std::move(result.root);
        std::string devmgr = data["data"]["devmgr"].asString();
        token_ = data["data"]["token"].asString();
        int ret = sscanf(devmgr.data(), "%[0-9.]:%hu", devmgr_server_ip_, &devmgr_server_port_);
        if (ret != 2) {
            errorf("get devmgr error, %s\n", data.toStyledString().data());
        }
        tracef("devmgr: %s:%d\n", devmgr_server_ip_, devmgr_server_port_);
    }
    sock_->stop();
    return true;
}

bool Huidian::device_login_mgr() {
    sock_ = std::make_shared<infra::TcpIO>();
    if (!sock_->connect(devmgr_server_ip_, devmgr_server_port_, false)) {
        errorf("tcp connect to %s:%d error\n", devmgr_server_ip_, devmgr_server_port_);
        return false;
    }
    tracef("connect mgr %s:%d succeed\n", devmgr_server_ip_, devmgr_server_port_);
    sock_->setNoblocked(true);
    sock_->setCallback([this](infra::Buffer& buffer) {
        onSocketRead(buffer);
    }, [this](int32_t fd) {
    });

    Json::Value data;
    data["token"] = token_;
    data["version"] = "4.0.0.1";
    data["login_cnt"] = login_count_++;
    data["device_type"] = 0;
    data["device_func"] = "JJ";  //AI_SKU_TYPE

    CallResult result = syncRequest("device_login_mgr_req", data);
    if (result.error_code != UlucuErrorCode::SUCCESS) {
        sock_->stop();
        return false;
    } else {
        connect_mgr_ = true;
        tracef("connect mgr %s:%d succ\n", devmgr_server_ip_, devmgr_server_port_);
        keep_alive_timer_.start(30 * 1000, [this]() -> bool {
            device_keepalive_mgr();
            return true;
        });
    }
    return true;
}

void Huidian::device_keepalive_mgr() {
    Json::Value data = Json::objectValue;
    std::string serial = std::to_string(infra::getCurrentTimeUs());
    infra::Buffer buffer = makeHdRequest("device_keepalive_mgr_req", data, serial.data());
    if (!buffer.empty()) {
        sendData(buffer);
    }
}

#define REGISTER_HUIDIAN_METHOND_FUNC(x) registerMethodFunc(#x, &HuidianHandler::x, &handler_)
void Huidian::initHandlerList() {
    REGISTER_HUIDIAN_METHOND_FUNC(get_device_version_req);
    REGISTER_HUIDIAN_METHOND_FUNC(get_device_information_req);
    REGISTER_HUIDIAN_METHOND_FUNC(open_debug_req);
}

int32_t Huidian::call(std::string key, Json::Value& data, Json::Value& result, std::string& reason) {
    int32_t error_code = 0;
    auto it = huidian_handlers_.find(key);
    if (it == huidian_handlers_.end()) {
        error_code = 10000;
        reason = "not support this method";
        errorf("not support method:%s\n", key.c_str());
    } else {
        return it->second(data, result, reason);
    }
    return error_code;
}

}