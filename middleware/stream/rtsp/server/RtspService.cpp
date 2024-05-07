/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspService.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:55:06
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <memory>
#include "rtsp/include/RtspService.h"
#include "RtspServiceImpl.h"

typedef struct RtspServiceInternal {
    std::shared_ptr<RtspServiceImpl> impl;
    RtspServiceInternal() : impl(std::make_shared<RtspServiceImpl>()) {}
} RtspServiceInternal;

RtspService* RtspService::instance() {
    static RtspService s_rtsp;
    return &s_rtsp;
}

RtspService::RtspService() : internal_(nullptr) {
}

RtspService::~RtspService() {
    if (internal_) {
        RtspServiceInternal* intenal = (RtspServiceInternal*)internal_;
        delete intenal;
    }
}

bool RtspService::start(int32_t port) {
    if (internal_ == nullptr) {
        internal_ = (void*)(new RtspServiceInternal());
    }
    return internal_ && ((RtspServiceInternal*)internal_)->impl->start(port);
}

bool RtspService::stop() {
    if (internal_ == nullptr) {
        return true;
    }
    RtspServiceInternal* intenal = (RtspServiceInternal*)internal_;
    intenal->impl->stop();
    delete intenal;
    internal_ = nullptr;
    return true;
}

bool RtspService::restart(int32_t port) {
    return internal_ && ((RtspServiceInternal*)internal_)->impl->restart(port);
}
