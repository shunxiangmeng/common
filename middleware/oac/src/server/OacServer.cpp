/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OACServer.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 17:14:42
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "OacServer.h"
#include "infra/include/Logger.h"
#include "hal/Video.h"
#include "infra/include/thread/WorkThreadPool.h"

namespace oac {

IOacServer* IOacServer::instance() {
    return OacServer::instance();
}

OacServer* OacServer::instance() {
    static OacServer s_oac_server;
    return &s_oac_server;
}

OacServer::OacServer() : image_manager_(ImageManager::Role::server), 
    rpc_server_(IPrivServer::instance()->rpcServer()) {
    
}

OacServer::~OacServer() {
}

bool OacServer::start(int32_t sub_channel, int32_t image_count) {
    alg_channel_ = sub_channel;
    hal::VideoEncodeParams video_encode;
    if (!hal::IVideo::instance()->getEncodeParams(0, sub_channel, video_encode)) {
        return false;
    }

    info_.shared_memory_path = "/oac_shared_memory";
    info_.shared_image_sem_prefix = "/oac_shared_image_";
    info_.width = *video_encode.width;
    info_.height = *video_encode.height;
    info_.format = IMAGE_PIXEL_FORMAT_RGB_888;
    info_.count = image_count;

    if (!image_manager_.init(info_)) {
        return false;
    }

    initServerMethod();

    return infra::Thread::start();
}

bool OacServer::stop() {
    return true;
}

void OacServer::run() {
    infof("OacServer thread start...\n");
    while (running()) {

        std::shared_ptr<SharedImage> image = image_manager_.getWriteSharedImage();

        //hal::IVideo::instance()->waitViImage(0, alg_channel_, 40);
        image->wait();

        image->shared_picture->busy = true;

        hal::IVideo::VideoImage video_image;
        video_image.buffer_size = image->shared_picture->buffer_size;
        video_image.data = image->data_addr;

        hal::IVideo::instance()->getViImage(0, alg_channel_, video_image);

        image->shared_picture->width = video_image.width;
        image->shared_picture->height = video_image.height;
        image->shared_picture->stride = video_image.stride;
        image->shared_picture->size = video_image.data_size;
        image->shared_picture->timestamp = video_image.timestamp;
        image->shared_picture->frame_number = video_image.frame_number;
        image->shared_picture->empty = false;
        image->shared_picture->busy = false;
        warnf("write index:%d, pts:%lld\n", image->shared_picture->index, image->shared_picture->timestamp);
        image->release();


        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }
    warnf("OacServer thread exit...\n");
}

void OacServer::initServerMethod() {
    rpc_server_.register_handler("on_alg_info", &OacServer::algInfo, this);
    rpc_server_.register_handler("shared_image_info", &OacServer::sharedImageInfo, this);
    rpc_server_.register_handler("on_current_detect_target", &OacServer::onCurrentDetectTarget, this);
}

void OacServer::algInfo(rpc_conn wptr, uint16_t alg_rpc_port) {
    tracef("alg server port:%d\n", alg_rpc_port);
    infra::WorkThreadPool::instance()->async([&, alg_rpc_port]() {
        priv_client_ = IPrivClient::create();
        if (!priv_client_->connect("127.0.0.1", alg_rpc_port)) {
            errorf("priv_client connect port:%d failed\n", alg_rpc_port);
            return; 
        }
        std::string alg_sdk_version = priv_client_->rpcClient().call<std::string>("alg_sdk_version");
        std::string alg_application_version = priv_client_->rpcClient().call<std::string>("alg_application_version");
        infof("alg_sdk_version:%s, alg_application_version:%s\n", alg_sdk_version.data(), alg_application_version.data());
    });
}

SharedImageInfo OacServer::sharedImageInfo(rpc_conn wptr) {
    tracef("sharedImageInfo+++\n");
    return info_;
}

void OacServer::onCurrentDetectTarget(rpc_conn wptr, std::string json_data) {
    tracef("onCurrentDetectTarget:%s\n", json_data.data());
}

}