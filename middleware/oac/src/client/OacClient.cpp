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
#include "jsoncpp/include/json.h"

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

bool OacClient::init(std::weak_ptr<IOacAlg> alg) {
    alg_ = alg;
    return true;
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
    infra::optional<SharedImageInfo> info = rpc_client_.call<SharedImageInfo>("shared_image_info");
    if (!info.has_value()) {
        errorf("get shared_image_info error\n");
        return false;
    }
    if (!image_manager_.init(*info)) {
        return false;
    }
    rpc_client_.call("on_alg_info", priv_server_port_);
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
    auto alg = alg_.lock();
    if (alg) {
        return alg->sdkVersion();
    }
    return "0.0.0.0";
}

std::string OacClient::algApplicationVersion(rpc_conn wptr) {
    auto alg = alg_.lock();
    if (alg) {
        return alg->version();
    }
    return "0.0.0.0";
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
    image.data = shared_image->data_addr;
    return true;
}

bool OacClient::releaseImageFrame(ImageFrame& image) {
    std::shared_ptr<SharedImage> shared_image = image_manager_.getSharedImageByIndex(image.index);
    shared_image->shared_picture->empty = true;
    shared_image->release();
    return true;
}

bool OacClient::pushDetectRegion(std::vector<DetectRegion>& detect_region) {
    
    return true;
}

bool OacClient::pushCurrentDetectTarget(CurrentDetectResult& result) {
    if (result.targets.size() == 0) {
        return false;
    }
    Json::Value root = Json::nullValue;
    root["timestamp"] = result.timestamp;
    for (auto &target: result.targets) {
        Json::Value item = Json::nullValue;
        item["type"] = (int32_t)target.shap_type;
        item["type"] = (int32_t)target.type;
        item["id"] = target.id;

        if (target.shap_type == E_TargetShapType_rect_pose || target.shap_type == E_TargetShapType_rect) {
            item["rect"]["x"] = target.rect.x;
            item["rect"]["y"] = target.rect.y;
            item["rect"]["w"] = target.rect.w;
            item["rect"]["h"] = target.rect.h;
        }

        for (auto &point : target.points) {
            Json::Value p = Json::nullValue;
            p["x"] = point.x;
            p["y"] = point.y;
            item["points"].append(p);
        }

        root["targets"].append(item);
    }

    Json::FastWriter writer;
    std::string str = writer.write(root);
    //tracef("str:%d, targets:%s\n", str.size(), str.data());
    rpc_client_.call("on_current_detect_target", std::move(str));
    return true;
};

}