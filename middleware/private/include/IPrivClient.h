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

    virtual bool testSyncCall() = 0;

    virtual RPCClient& rpcClient() = 0;

};