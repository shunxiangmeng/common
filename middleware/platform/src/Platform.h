/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Platform.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-17 19:41:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "platform/include/IPlatform.h"

namespace platform {

class Platform : public IPlatform {
public:
    Platform();
    ~Platform();

    virtual bool init(DeviceAttribute *device_attribute) override;

};


}