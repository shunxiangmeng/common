/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IUserManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:14:10
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <vector>
#include "jsoncpp/include/json.h"

typedef struct {
    std::string username;
    std::string password;
    std::string authority_type;
    std::string realm;
    std::string nonce;
    std::string password_type;
    std::string authority_info;
} LoginInfo;

class IUserManager {
public:
    static IUserManager* instance();

    typedef struct {
        std::string username;
    } UserInfo;

    virtual bool initial(const char* user_config_file_name) = 0;
    virtual bool addUser(const UserInfo& user) = 0;
    virtual bool deleteUser(const std::string& username) = 0;
    virtual bool modifyUser(const UserInfo& user) = 0;
    virtual	bool modifyPassword(const std::string& username, const std::string& password, const std::string& old_password) = 0;
    virtual bool getUserInfo(const std::string& userName, UserInfo& curUser) = 0;
    virtual bool getUserInfoAll(std::vector<UserInfo>& users) = 0;
    virtual bool getOnlineUsers(Json::Value& userinfo) = 0;
    virtual bool checkPassword(const LoginInfo& si) = 0;
    virtual bool setAdminPassword(const std::string& password, const std::string& contactInfo = std::string()) = 0;
    virtual bool resetAdminPassword(const Json::Value& param) = 0;
};