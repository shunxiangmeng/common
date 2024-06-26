/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HuidianHandler.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-26 16:35:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "HuidianHandler.h"
#include "infra/include/Logger.h"

namespace ulucu {

int32_t HuidianHandler::get_device_version_req(Json::Value& data, Json::Value& result, std::string& reason) {
    result["adver"] = "4.0.0.1";   //固件版本号
    result["rootver"] = "4.0.0.1";   //固件版本号
    result["version"] = "4.0.0.1";
    result["devsn"] = "4.0.0.1";
    result["ulusn"] = "4.0.0.1";
    result["builder"] = "4.0.0.1";
    result["function"] = "4.0.0.1";  //AI_SKU_TYPE
    return 0;
}

}
