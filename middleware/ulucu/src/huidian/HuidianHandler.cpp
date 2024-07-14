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
#include "infra/include/network/Network.h"
#include <sys/statfs.h>

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

int32_t HuidianHandler::get_device_information_req(Json::Value& data, Json::Value& result, std::string& reason) {

    struct statfs fs_info = {0};

    Json::Value interfaces = Json::arrayValue;
    std::vector<infra::Interface> interface_list = infra::getInterfaceList();
    for (auto &it : interface_list) {
        Json::Value interface;
        interface["nmae"] = it.name;
        interface["ip"] = it.ip;
        interface["netmask"] = it.netmask;
        interface["broadaddr"] = it.broadaddr;
        interfaces.append(interface);
    }

    result["device_name"] = "IPC";
    result["device_type"] = 0;   // 0 - ipc, 1 - nvr, 2 - dvr
    result["device_ip"] = interface_list[0].ip;
    result["device_mac"] = "todo...";
    result["device_DNS"] = "todo...";
    result["device_interfaces"] = interfaces;

    result["oem_device_type"] = "todo...";
    result["oem_device_series_number"] = "todo...";
    result["oem_master_version"] = "todo...";
    result["oem_encoding_version"] = "todo...";

    result["channel_count"] = 1;
    result["harddisk_count"] = 0;
    result["io_input_count"] = 0;
    result["io_output_count"] = 0;

    result["soc_temprature"] = 0;

    return 0;
}

}
