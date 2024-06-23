/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Ulucu.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-22 13:43:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "ulucu/include/IUlucu.h"
#include "anyan/src/inc/Anyan_Device_SDK.h"

namespace ulucu {

class Ulucu : public IUlucu {
public:
    Ulucu();
    ~Ulucu();

    virtual bool init() override;
    
private:

};

}