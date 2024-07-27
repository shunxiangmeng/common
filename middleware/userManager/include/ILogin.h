/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ILogin.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:49:22
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include "IUserManager.h"

class ILogin {
public:

    typedef struct {
        std::string authority_type;
        std::string realm;
        std::string nonce;  //random
    } LoginChallenge;

    static ILogin* instance();

    virtual bool loginFirst(LoginChallenge& challenge) = 0;
    virtual bool loginAgain(const LoginInfo& login_info) = 0;
};