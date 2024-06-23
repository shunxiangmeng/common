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
#include <memory>
#include "ulucu/include/IUlucu.h"
#include "huidian/huidian.h"
#include "anyan/src/inc/Anyan_Device_SDK.h"

namespace ulucu {

class Ulucu : public IUlucu {
public:
    Ulucu();
    ~Ulucu();

    virtual bool init() override;

    static Ulucu* instance();

    void anyanInteractCallback(void* args);
private:
    void initHuidian();

private:
    std::shared_ptr<Huidian> huidian_;
    std::string device_sn_;
};

}