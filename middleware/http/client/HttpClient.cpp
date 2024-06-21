/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpClient.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-06-18 11:24:14
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "HttpClient.h"

std::shared_ptr<IHttpClient> IHttpClient::create() {
    return std::make_shared<HttpClient>();
}

HttpClient::HttpClient(/* args */) {
}

HttpClient::~HttpClient() {

}

bool HttpClient::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    if (curl_ == nullptr) {
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);

    return true;
}
