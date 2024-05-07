/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Transport.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 13:49:30
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <stdint.h>
#include <string>
#include "Transport.h"
#include "TransportTcp.h"
#include "TransportUdp.h"
#include "infra/include/Logger.h"

std::shared_ptr<Transport> Transport::create(SocketType sockType, std::shared_ptr<infra::Socket> &sock, bool needClose) {
    if (!sock) {
        errorf("create error, sock.get() is nullptr\n");
        return nullptr;
    }
    if (sockType == socketTypeTcp) {
        return std::make_shared<TransportTcp>(sock, needClose);
    } else if (sockType == socketTypeUdp) {
        return std::make_shared<TransportUdp>(sock, needClose);
    } else {
        errorf("not support sockType:%d\n", sockType);
    }
    return nullptr;
}

Transport::Transport() : mChannelId(-1), mPort(0) {
}

Transport::~Transport(){
}

int32_t Transport::setDataCallback(DataProc dataCallback){
    mDataCallback = dataCallback;
    return 0;
}

int32_t Transport::setExceptionCallback(ExceptionCallBack callback) {
    exception_callback_ = callback;
    return 0;
}
