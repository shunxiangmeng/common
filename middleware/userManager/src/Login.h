/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Login.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:49:59
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "userManager/include/ILogin.h"

class Login : public ILogin {
public:
    Login() = default;
    ~Login() = default;

    virtual bool loginFirst(LoginChallenge& challenge) override;
    virtual bool loginAgain(const LoginInfo& si) override;
};
