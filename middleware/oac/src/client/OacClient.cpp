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
    : running_(false), image_manager_(ImageManager::Role::client),
    priv_client_(IPrivClient::create()), rpc_client_(priv_client_->rpcClient()),
    priv_server_(IPrivServer::create()), rpc_server_(priv_server_->rpcServer()) {
    priv_server_port_ = 7001;
}

OacClient::~OacClient() {
}

bool OacClient::start() {
    if (!priv_server_->start(priv_server_port_)) {
        return false;
    }
    initRpcServerMethod();

    if (!priv_client_->connect("127.0.0.1", 7000)) {
       errorf("priv_client connect failed\n");
       return false; 
    }
    SharedImageInfo info = rpc_client_.call<SharedImageInfo>("shared_image_info");

    if (!image_manager_.init(info)) {
        return false;
    }
    return true;
}

bool OacClient::stop() {
    return false;
}

void OacClient::initRpcServerMethod() {
    rpc_server_.register_handler("alg_sdk_version", &OacClient::algVersion, this);
    rpc_server_.register_handler("alg_application_version", &OacClient::algApplicationVersion, this);
}

std::string OacClient::algVersion(rpc_conn wptr) {
    return "0.7.1.999";
}
std::string OacClient::algApplicationVersion(rpc_conn wptr) {
    return "1.0.0.2";
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


}