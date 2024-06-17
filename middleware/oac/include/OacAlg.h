/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacAlg.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-17 15:09:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>

namespace oac {

class IOacAlg : public std::enable_shared_from_this<IOacAlg> {
public:
    virtual std::string version() = 0;
    virtual std::string sdkVersion() = 0;
};

}