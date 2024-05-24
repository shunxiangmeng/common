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

private:
    int32_t alg_channel_;
    ImageManager image_manager_;
};


}