/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  TransportChannelInterleave.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 15:47:11
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <thread>
#include "infra/include/Logger.h"
#include "TransportChannelInterleave.h"
#include "infra/include/network/Defines.h"

TransportChannelInterleave::TransportChannelInterleave() : mTransport(nullptr), mSentPackets(0), mSentBytes(0) {
    mTransMode = TransportModeInterleaved;
}

TransportChannelInterleave::~TransportChannelInterleave() {
    tracef("~TransportChannelInterleave()\n");
}

int32_t TransportChannelInterleave::setChannelSock(std::shared_ptr<infra::Socket> sock, bool needClose) {
    if (mTransport) {
        errorf("sock has been set already\n");
        return -1;
    }
    auto transport = Transport::create(Transport::socketTypeTcp, sock, needClose); 
    if (!transport){
        errorf("create transport error\n");
        return -1;
    }
    mTransport = transport;
    return 0;
}

int32_t TransportChannelInterleave::setDataCallback(DataProc callback) {
    if (mTransport == nullptr) {
        return -1;
    }
    return mTransport->setDataCallback(callback);
}

int32_t TransportChannelInterleave::setExceptionCallback(ExceptionCallBack callback) {
    if (mTransport == nullptr) {
        return -1;
    }
    return mTransport->setExceptionCallback(callback);
}

bool TransportChannelInterleave::start() {
    if (mTransport == nullptr) {
        return false;
    }
    return mTransport->startReceive();
}

bool TransportChannelInterleave::stop() {
    if (mTransport == nullptr) {
        return false;
    }
    return mTransport->stopReceive();
}

int32_t TransportChannelInterleave::sendCommand(const char* data, int32_t len) {
    if (mTransport == nullptr) {
        return 0;
    }
    int32_t ret = mTransport->send(data, len);
    if (ret < 0) {
        errorf("send ret:%d\n", ret);
        // todo mTransport->setBroken(true);
    }
    return ret;
}

int32_t TransportChannelInterleave::sendMedia(int32_t channelId, const MediaFrame &frame, int32_t mark) {
    if (mTransport == nullptr) {
        return 0;
    }
    int32_t sendLen = 0;
    int32_t dataLen = frame.size();
    char *buffer = (char*)frame.data();
    int32_t packLen = 0;
    struct RtpTcpHdr *hdr;
    int32_t send0Count = 0;

    do {
        hdr = (struct RtpTcpHdr*)(buffer + sendLen);
        packLen = sizeof(struct RtpTcpHdr) + ntohs(hdr->len);
        int32_t ret = mTransport->send((const char*)(buffer + sendLen), packLen);
        if (ret < 0) {
            errorf("CTransportChannelIndepent send rtp failed, ret:%d\n", ret);
            return ret;
        }  else if (ret == 0) {
            send0Count++;
            warnf("sendMedia send ret %d, send0Count:%d, packLen:%d\n", ret, send0Count, packLen);
            if (send0Count >= 10) {
                return -1;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else if (ret < packLen) {
            warnf("sendMedia send ret %d < packLen:%d\n", ret, packLen);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            int32_t ret1 = mTransport->send((const char*)(buffer + sendLen + ret), packLen - ret);
            tracef("sendMedia send ret %d, to send len %d\n", ret1, packLen - ret);
            if (ret1 < packLen - ret) {
                errorf("lost data\n");
            }
            ret = packLen;
        }

        sendLen += ret;
        mSentBytes += ret;
        if (ret > 0) {
            mSentPackets++;
        }

    } while (sendLen < dataLen);
    return sendLen;
}

void TransportChannelInterleave::getSendStatistic(int32_t channelId, uint32_t &sendPackets, uint32_t &sendBytes) {
    sendBytes = mSentBytes;
    sendPackets = mSentPackets;
}
