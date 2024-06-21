/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-18 11:23:32
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "http/include/IHttpClient.h"
#define CURL_STATICLIB
#include "prebuild/include/curl/curl.h"

class HttpClient : public IHttpClient {
public:
    HttpClient();
    ~HttpClient();

    virtual bool init() override;

private:
    CURL* curl_ = nullptr;
};

