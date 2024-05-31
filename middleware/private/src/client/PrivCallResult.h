/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivCallResult.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-31 19:46:19
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <string>
#include <future>
#include "jsoncpp/include/json.h"

enum class PrivErrorCode {
    SUCCESS = 0,
    TIMEOUT = 1,
    FAILED = 2,
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
            error_message = std::move(other.error_message);
            data = std::move(other.data);
        }
        return *this;
    }

    bool success() const {
        return error_code == PrivErrorCode::SUCCESS;
    }

private:
    CallResult(const CallResult&) = delete;
    CallResult& operator=(const CallResult& other) = delete;

public:
    PrivErrorCode error_code = PrivErrorCode::FAILED;
    std::string error_message;
    Json::Value data;
};

using AsyncCallResult = std::pair<uint32_t, std::future<CallResult>>;
