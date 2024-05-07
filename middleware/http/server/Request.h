/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  request.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 21:51:06
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <string>
#include "infra/include/Buffer.h"
#include "infra/include/network/TcpSocket.h"
#include "http/common/picohttpparser.h"

class Request {
public:
    Request();
    ~Request();

    int32_t readRequest(std::shared_ptr<infra::TcpSocket> &sock);
    bool parse();

    std::string method() const;
    std::string url() const;
    std::string getHeaderValue(std::string key) const;
    std::string getQueryValue(std::string key);
    
    const std::map<std::string, std::string>& queries() {
        return queries_;
    }

    std::string body() const {
        return {buffer_.data() + header_len_, body_len_};
    };

    size_t totalLength() {
        return header_len_ + body_len_;
    }

    size_t headerLength() const {
        return header_len_;
    }

    size_t bodyLength() const {
        return  body_len_;
    }

    bool isHttp11();
    void reset();
private:
    void parseQuery(std::string str);
private:
    infra::Buffer buffer_;

    std::string url_;
    std::string method_;
    int minor_version_;
    size_t headers_num_ = 0;
    struct phr_header headers_[32];
    int header_len_;
    size_t body_len_;
    char *body_str_ = nullptr;

    bool is_chunked_ = false;
    std::map<std::string, std::string> queries_;
};