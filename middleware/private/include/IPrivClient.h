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
#include "rpc/RpcClient.h"
#include "infra/include/Signal.h"
#include "common/mediaframe/MediaFrame.h"
#include "jsoncpp/include/json.h"

using EventFunction = std::function<void(Json::Value&)>;

class IPrivClient {
public:
typedef infra::TSignal<void, MediaFrameType, MediaFrame&> PrivClientSignal;
typedef PrivClientSignal::Proc OnFrameProc;

    IPrivClient() = default;
    virtual ~IPrivClient() = default;

    static std::shared_ptr<IPrivClient> create();

    virtual bool connect(const char* server_ip, uint16_t server_port) = 0;

    virtual bool startPreview(int32_t channel, int32_t sub_channel, OnFrameProc onframe) = 0;
    virtual bool stopPreview(OnFrameProc onframe) = 0;
    virtual bool getVideoFormat(std::string &format) = 0;
    virtual bool setVideoFormat(std::string &format) = 0;
    virtual bool getVideoConfig(Json::Value &video_config) = 0;
    virtual bool setVideoConfig(Json::Value &video_config) = 0;

    virtual bool subscribeEvent(const char* event, EventFunction event_callback) = 0;

    virtual bool testSyncCall() = 0;

    virtual RPCClient& rpcClient() = 0;

};