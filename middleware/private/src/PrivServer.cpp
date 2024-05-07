/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-12 22:02:57
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivServerInternal.h"
#include "private/include/PrivServer.h"

typedef struct PrivServiceInternal {
    std::shared_ptr<PrivInternal> impl;
    PrivServiceInternal() : impl(std::make_shared<PrivInternal>()) {}
} PrivServiceInternal;

PrivServer* PrivServer::instance() {
    static PrivServer s_priv;
    return &s_priv;
}

PrivServer::PrivServer() : internal_(nullptr) {
}

PrivServer::~PrivServer() {
    if (internal_) {
        PrivServiceInternal* intenal = (PrivServiceInternal*)internal_;
        delete intenal;
        internal_ = nullptr;
    }
}

bool PrivServer::start(int32_t port) {
    if (internal_ == nullptr) {
        internal_ = (void*)(new PrivServiceInternal());
    }
    return internal_ && ((PrivServiceInternal*)internal_)->impl->start(port);
}

bool PrivServer::stop() {
    return internal_ && ((PrivServiceInternal*)internal_)->impl->stop();
}

bool PrivServer::restart(int32_t port){
    if (internal_ == nullptr) {
        return false;
    }
    if (((PrivServiceInternal*)internal_)->impl->stop()){
        return ((PrivServiceInternal*)internal_)->impl->start(port);
    }
    return false;
}

bool PrivServer::addSubscribeEvent(const char* name) {
    if (internal_ == nullptr || name == nullptr) {
        return false;
    }
    return ((PrivServiceInternal*)internal_)->impl->addSubscribeEvent(name);    
}

bool PrivServer::sendEvent(const char* name, const Json::Value &event) {
    if (internal_ == nullptr) {
        return false;
    }
    return ((PrivServiceInternal*)internal_)->impl->sendEvent(name, event);
}
