/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  result.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-23 19:00:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <future>
#include <memory>
#include "jsoncpp/include/json.h"

namespace ulucu {

enum class UlucuErrorCode {
    SUCCESS = 0,
    SENDFAILED,
    TIMEOUT,
    FAILED,
};

struct CallResult {
public:
    CallResult() = default;
    CallResult(CallResult&& other) noexcept {
        *this = std::move(other);
    }

    CallResult& operator=(CallResult&& other) noexcept {
        if (this != &other) {
            error_code = std::move(other.error_code);
            code = other.code;
            reason = std::move(other.reason);
            data = std::move(other.data);
            root = std::move(other.root);
        }
        return *this;
    }

    bool success() const {
        return error_code == UlucuErrorCode::SUCCESS && code == 0;
    }

private:
    CallResult(const CallResult&) = delete;
    CallResult& operator=(const CallResult& other) = delete;

public:
    UlucuErrorCode error_code = UlucuErrorCode::FAILED;
    int32_t code;   /* 平台返回的错误码 */
    std::string reason;
    std::string data;
    Json::Value root;
};

using UlucuAsyncCallResult = std::pair<std::string, std::future<CallResult>>;

}