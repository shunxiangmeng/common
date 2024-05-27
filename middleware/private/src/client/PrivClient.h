/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivClient.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-27 09:59:19
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include "../Defs.h"
#include "private/include/IPrivClient.h"
#include "infra/include/network/SocketHandler.h"
#include "infra/include/network/TcpSocket.h"
#include "infra/include/Buffer.h"

class PrivClient : public IPrivClient, public infra::SocketHandler {
public:
    PrivClient();
    virtual ~PrivClient();

    virtual bool connect(const char* server_ip, uint16_t server_port) override;
private:
    /**
     * @brief scoket数据输入回调
     * @param fd
     * @return
     */
    virtual int32_t onRead(int32_t fd) override;
    /**
     * @brief socket异常回调
     * @param fd
     * @return
     */
    virtual int32_t onException(int32_t fd) override;

    std::shared_ptr<Message> parse();
    void process(std::shared_ptr<Message> &request);

private:
    std::string server_ip_;
    uint16_t server_port_;
    std::shared_ptr<infra::TcpSocket>   sock_;

    infra::Buffer buffer_;

};