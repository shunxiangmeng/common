/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UserManger.h
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:15:32
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include <map>
#include <mutex>
#include "userManager/include/IUserManager.h"

class UserManager : public IUserManager {
public:
    UserManager();
    ~UserManager();

    virtual bool initial(const char* user_config_file_path) override;
    virtual bool addUser(const IUserManager::UserInfo& user) override;
    virtual bool deleteUser(const std::string& username) override;
    virtual bool modifyUser(const IUserManager::UserInfo& user) override;
    virtual	bool modifyPassword(const std::string& username, const std::string& password, const std::string& old_password) override;
    virtual bool getUserInfo(const std::string& userName, IUserManager::UserInfo& curUser) override;
    virtual bool getUserInfoAll(std::vector<IUserManager::UserInfo>& users) override;
    virtual bool getOnlineUsers(Json::Value& userinfo) override;
    virtual bool checkPassword(const LoginInfo& login_info) override;
    virtual bool setAdminPassword(const std::string& password, const std::string& contactInfo = std::string()) override;
    virtual bool resetAdminPassword(const Json::Value& param) override;

private:
    bool findUser(const std::string& userName);
    std::string md5Hexstr(const std::string& username, const std::string& orig_password, const std::string& nonce, const std::string& authorization);

private:
    typedef std::vector<std::string> AuthorityVec;

    enum UserLevel {
        Administrator = 0,	//管理员
        Operator,			//操作员
        User,				//用户
        MaxLevel
    };

    struct UserInfo {
        std::string username;               // 用户名
        std::string password;               // 密码
        std::string comment;                // 备注信息
        AuthorityVec authority_list;        // 权限列表
        AuthorityVec cfg_authority_list;	// 配置权限
        UserLevel user_level;               // 组名称
        std::string contactInfo;            // 用户联系方式,只有admin用户才有该字段
        std::string checkcode;	            // 加密后的校验码，只有admin用户会用到这个字段，60秒存活期内非空
    };

    struct OnlineUserInfo {
        std::string username;
        UserLevel user_level;
        std::string client_type;
        std::string client_address;
        std::string login_time;
    };

    std::recursive_mutex account_mutex_;
    std::map<std::string, UserInfo> account_map_;

    std::mutex online_users_mutex_;
    std::map<int, OnlineUserInfo> online_users_;

    std::string config_path_;
    std::string backup_Path_;
};
