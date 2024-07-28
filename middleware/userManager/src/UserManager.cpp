/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UserManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-27 13:17:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "UserManager.h"
#include "infra/include/Logger.h"
#include "infra/include/MD5.h"
#include "infra/include/File.h"

IUserManager* IUserManager::instance() {
    static UserManager s_user_manager;
    return &s_user_manager;
}

UserManager::UserManager() {
}

UserManager::~UserManager() {
}

bool UserManager::initial(const char* user_config_file_name) {
    if (user_config_file_name == nullptr) {
        return false;
    }
    config_path_ = user_config_file_name;

    Json::Reader reader;
    std::string stream = infra::File::loadFile(config_path_);
    if (stream.length() == 0) {
        errorf("load user file(%s) fail\n", config_path_.c_str());
        return false;
    }
    Json::Value content;
    if (!reader.parse(stream, content)) {
        errorf("parse user file error\n");
        return false;
    }

    if (!content.isArray()) {
        errorf("user file content error\n");
        return false;
    }

    for (auto &it : content) {
        UserInfo user{};
        user.username = it["username"].asString();
        user.password = it["password"].asString();
        account_map_[user.username] = user;
    }
    return true;
}

bool UserManager::addUser(const IUserManager::UserInfo& user) {
    return false;
}

bool UserManager::deleteUser(const std::string& username) {
    return false;
}

bool UserManager::modifyUser(const IUserManager::UserInfo& user) {
    return false;
}

bool UserManager::modifyPassword(const std::string& username, const std::string& password, const std::string& old_password) {
    return false;
}

bool UserManager::getUserInfo(const std::string& userName, IUserManager::UserInfo& curUser) {
    return false;
}

bool UserManager::getUserInfoAll(std::vector<IUserManager::UserInfo>& users) {
    return false;
}

bool UserManager::getOnlineUsers(Json::Value& userinfo) {
    return false;
}

bool UserManager::checkPassword(const LoginInfo& login_info) {
    const std::string &username = login_info.username;
    const std::string &password = login_info.password;
    const std::string &nonce = login_info.nonce;
    const std::string &authority_info = login_info.authority_info;
    
    if (!findUser(username)) {
        warnf("username %s is not existed\n", username.data());
        return false;
    }

    std::string original_password = account_map_[username].password;
    if (login_info.authority_type == "Digest") {
        return (md5Hexstr(username, original_password, nonce, authority_info) == password);
    } else if (login_info.authority_type == "Plain") {
        return login_info.password == original_password;
    } else if (login_info.password_type == "Onvif") {

    }
    return false;
}

bool UserManager::setAdminPassword(const std::string& password, const std::string& contactInfo) {
    return false;
}

bool UserManager::resetAdminPassword(const Json::Value& param) {
    return false;
}

bool UserManager::findUser(const std::string& username) {
    std::lock_guard<std::recursive_mutex> guard(account_mutex_);
    return account_map_.find(username) != account_map_.end();
}

std::string UserManager::md5Hexstr(const std::string& username, const std::string& orig_password, const std::string& nonce, const std::string& authorization) {
    auto calculateMd5 = [](std::string buffer) -> std::string {
        infra::MD5 md5;
        md5.update(buffer);
        return md5.finalHexString();
    };
    // md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
    std::string realm = "Login to bronco";
    std::string A1(username + ":" + realm + ":" + orig_password);
    std::string H1 = calculateMd5(A1);
    std::string calValue(H1 + ":" + nonce + ":" + authorization);
    return calculateMd5(calValue);
}