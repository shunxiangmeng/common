/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IUlucu.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-22 13:44:31
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>

namespace ulucu {

class IUlucu {
public:
    static IUlucu* instance();

    virtual bool init() = 0;
};

} // namespace ulucu
