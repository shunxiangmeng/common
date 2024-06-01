/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacClient.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-24 16:38:03
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <thread>
#include <chrono>
#include "OacClient.h"
#include "infra/include/Logger.h"

namespace oac {

IOacClient* IOacClient::instance() {
    return OacClient::instance();
}

IOacClient* OacClient::instance() {
    static OacClient s_oac_client;
    return &s_oac_client;
}

OacClient::OacClient() 
    : image_manager_(ImageManager::Role::client),
    priv_client_(IPrivClient::create()), rpc_client_(priv_client_->rpcClient()) {
}

OacClient::~OacClient() {
}


bool OacClient::start() {
    if (!priv_client_->connect("127.0.0.1", 7000)) {
       errorf("priv_client connect failed\n");
       return false; 
    }
    SharedMemoryInfo info = rpc_client_.call<SharedMemoryInfo>("shared_memory_info");

    if (!image_manager_.init(640, 480, IMAGE_PIXEL_FORMAT_RGB_888, 3)) {
        return false;
    }
    return true;
}

bool OacClient::stop() {
    return false;
}

bool OacClient::getImageFrame(ImageFrame& image) {
    std::shared_ptr<SharedImage> shared_image = image_manager_.getReadSharedImage();
    while (shared_image->shared_picture->empty) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    shared_image->wait();

    image.index = shared_image->shared_picture->index;
    image.format = shared_image->shared_picture->format;
    image.width = shared_image->shared_picture->width;
    image.height = shared_image->shared_picture->height;
    image.stride = shared_image->shared_picture->stride;
    image.size = shared_image->shared_picture->size;
    image.timestamp = shared_image->shared_picture->timestamp;
    image.frame_number = shared_image->shared_picture->frame_number;
    image.data = shared_image->shared_picture->data;
    return true;
}

bool OacClient::releaseImageFrame(ImageFrame& image) {
    std::shared_ptr<SharedImage> shared_image = image_manager_.getSharedImageByIndex(image.index);
    shared_image->shared_picture->empty = true;
    shared_image->release();
    return true;
}

SharedMemoryInfo OacClient::sharedMemoryInfo() {
    SharedMemoryInfo info;
    return info;
}

}