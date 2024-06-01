/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  OacClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-22 20:06:23
 * Description :  oac消息传递依赖私有协议中的rpc
 * Note        : 
 ************************************************************************/
#pragma once
#include "oac/include/OacClient.h"
#include "../common/ImageManager.h"
#include "../common/Message.h"
#include "private/include/IPrivClient.h"

namespace oac {
class OacClient : public IOacClient {
    OacClient();
    ~OacClient();
public:
    static IOacClient* instance();

    virtual bool start() override;
    virtual bool stop() override;

    virtual bool getImageFrame(ImageFrame& image) override;
    virtual bool releaseImageFrame(ImageFrame& image) override;   //使用完成之后需要调用此接口释image

private:
    ImageManager image_manager_;
    std::shared_ptr<IPrivClient> priv_client_;
    RPCClient &rpc_client_;

};

}