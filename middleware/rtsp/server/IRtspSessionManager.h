/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IRtspSessionManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 23:48:25
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>

class RtspSession;
class IRtspSessionManager {
public:
    IRtspSessionManager() = default;
    virtual ~IRtspSessionManager() {}
    virtual int32_t remove(std::shared_ptr<RtspSession> session) = 0;
    virtual bool isAuthority() = 0;
};
