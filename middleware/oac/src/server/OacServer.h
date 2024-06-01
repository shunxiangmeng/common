/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacServer.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 20:10:56
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "oac/include/OacServer.h"
#include "infra/include/thread/Thread.h"
#include "../common/ImageManager.h"
#include "../common/Message.h"
#include "private/include/IPrivServer.h"
#include "private/include/IPrivClient.h"

namespace oac {

class OacServer : public IOacServer, public infra::Thread {
    OacServer();
    ~OacServer();
public:
    static OacServer* instance();

    virtual bool start(int32_t sub_channel = 1, int32_t image_count = 3) override;
    virtual bool stop() override;

private:
    virtual void run() override;
    void initServerMethod();

    //rpc method
    void algInfo(rpc_conn wptr, uint16_t alg_rpc_port);
    SharedImageInfo sharedImageInfo(rpc_conn wptr);

private:
    SharedImageInfo info_;
    int32_t alg_channel_;
    ImageManager image_manager_;
    RPCServer &rpc_server_;
    std::shared_ptr<IPrivClient> priv_client_;
};


}