/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IPrivClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-26 22:57:52
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include <chrono>

class IPrivClient {
public:
    IPrivClient() = default;
    virtual ~IPrivClient() = default;

    static std::shared_ptr<IPrivClient> create();

    virtual bool connect(const char* server_ip, uint16_t server_port) = 0;


    virtual bool testSyncCall() = 0;


};