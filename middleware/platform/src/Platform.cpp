/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Platform.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-17 19:33:44
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Platform.h"
#include "infra/include/Logger.h"

namespace platform {

std::shared_ptr<IPlatform> IPlatform::create() {
    return std::make_shared<Platform>();
} 

Platform::Platform() {
}

Platform::~Platform() {
}

bool Platform::init(DeviceAttribute *device_attribute) {


    return true;
}

}