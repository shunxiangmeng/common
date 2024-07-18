/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HuidianHandler.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-26 16:35:30
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "jsoncpp/include/json.h"

namespace ulucu {

class HuidianHandler {
public:
    HuidianHandler() = default;
    ~HuidianHandler() = default;

    int32_t get_device_version_req(Json::Value& data, Json::Value& result, std::string& reason);
    int32_t get_device_information_req(Json::Value& data, Json::Value& result, std::string& reason);
    int32_t open_debug_req(Json::Value& data, Json::Value& result, std::string& reason);


};

}
