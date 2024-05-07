/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  reply.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 23:01:22
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <vector>
#include "http/common/Defines.h"

enum class statusType {
    init,
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
};

class Reply {
public:
    Reply() = default;
    ~Reply() = default;

    void addHeader(std::string& key, std::string& value);
    void addHeader(std::string key, std::string& value);
    void addHeader(std::string&& key, std::string&& value);
    void setContentType(std::string&& content);
    void setContent(std::string&& content);
    void setStatus(statusType status);

    std::string toBuffer();
    void reset();

private:
    std::string raw_url_;
    std::vector<std::pair<std::string, std::string>> headers_;

    std::string content_;
    statusType status_  = statusType::init;
    ContentType body_type_ = ContentType::unknown;

    bool delay_ = false;
    std::string domain_;
    std::string path_;
};