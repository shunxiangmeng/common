/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IHttpClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-06-18 11:22:24
 * Description :  使用curl封装的 http client
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>

class IHttpClient {
public:
    static std::shared_ptr<IHttpClient> create();

    virtual bool init() = 0;
};