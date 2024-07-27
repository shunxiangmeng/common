/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Login.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:50:30
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Login.h"
#include <time.h>

ILogin* ILogin::instance() {
    static Login s_login;
    return &s_login;
}

bool Login::loginFirst(LoginChallenge& challenge) {
    static time_t tm = 0;
    if (tm <= 0) {
        time(&tm);
        srand((unsigned int)tm);
    }
    challenge.authority_type = "Digest";
    challenge.realm = "Login to bronco";
    challenge.nonce = std::to_string(rand());
    return true;
}

bool Login::loginAgain(const LoginInfo& login_info) {
    return IUserManager::instance()->checkPassword(login_info);
}
