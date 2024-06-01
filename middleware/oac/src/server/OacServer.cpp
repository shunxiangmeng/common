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
        video_image.data = image->shared_picture->data;

        hal::IVideo::instance()->getViImage(0, alg_channel_, video_image);

        image->shared_picture->width = video_image.width;
        image->shared_picture->height = video_image.height;
        image->shared_picture->stride = video_image.stride;
        image->shared_picture->size = video_image.data_size;
        image->shared_picture->timestamp = video_image.timestamp;
        image->shared_picture->frame_number = video_image.frame_number;
        image->shared_picture->empty = false;
        image->shared_picture->busy = false;
        //warnf("write index:%d, pts:%lld\n", image->shared_picture->index, image->shared_picture->timestamp);
        image->release();


        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }
    warnf("OacServer thread exit...\n");
}

void OacServer::initServerMethod() {
    rpc_server_.register_handler("shared_memory_info", &OacServer::sharedImageInfo, this);
}

SharedImageInfo OacServer::sharedImageInfo(rpc_conn wptr) {
    return info_;
}

}