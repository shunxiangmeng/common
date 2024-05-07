/************************************************
 * Copyright(c) 2021 uni-ubi
 * 
 * Project:    Rtsp
 * FileName:   TransportChannelIndepent.cpp
 * Author:     jinlun
 * Email:      jinlun@uni-ubi.com
 * Version:    V1.0.0
 * Date:       2021-02-07
 * Description: 
 * Others:
 *************************************************/
#include <chrono>
#include "TransportChannelIndepent.h"
#include "infra/include/network/Defines.h"
#include "infra/include/Logger.h"

TransportChannelIndepent* TransportChannelIndepent::create() {
    return new TransportChannelIndepent();
}

TransportChannelIndepent::TransportChannelIndepent() {
    mTransMode = TransportModeIndependent;
    for (auto &it : mSentPackets) {
        it = 0;
    }
    for (auto &it : mSentBytes) {
        it = 0;
    }    
}

TransportChannelIndepent::~TransportChannelIndepent() {
}

int32_t TransportChannelIndepent::addDataChannel(int32_t channelId, 
    std::shared_ptr<infra::Socket> rtpsock, 
    std::shared_ptr<infra::Socket> rtcpsock,
    const char* remoteIp, int32_t remotertpPort, int32_t remotertcpPort, bool needClose) {
    infof("addDataChannel channelId:%d, ip:%s, rtpPort:%d, rtcpPort:%d\n", 
        channelId, remoteIp, remotertpPort, remotertcpPort);
    if (channelId < 0 || !rtpsock || !rtcpsock || remoteIp == nullptr || remotertpPort < 0 || remotertcpPort < 0) {
        errorf("addDataChannel error\n");
        return -1;
    }

    auto createTransport = [&](int32_t index, std::shared_ptr<infra::Socket> sock, int32_t port) {
        auto transport = Transport::create(Transport::socketTypeUdp, sock, needClose);
        if (!transport) {
            errorf("create transport error\n");
            return transport;
        }
        if (strstr(remoteIp, "") != 0 && port != 0) {
            struct sockaddr_in addr = { 0 };
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(remoteIp);
            int32_t ret = transport->setOption(transportOptRemoteAddr, &addr, sizeof(addr));
            if (ret < 0) {
                errorf("setRemoteAddr error, ret:%d\n", ret);
            }
        } else {
            errorf("setRemoteAddr error\n");
            return transport;
        }
        int32_t send_buffer = 2 * 1024 * 1024;
        int32_t ret = transport->setOption(transportOptSndBuffer, (void*)&send_buffer, sizeof(send_buffer));
        if (ret < 0) {
            errorf("set send buffer error, ret:%d\n", ret);
        }
        transport->setChannelId(index);
        return transport;
    };
    mTransports[channelId].rtpTransport = createTransport(channelId, rtpsock, remotertpPort);
    mTransports[channelId].rtcpTransport = createTransport(channelId, rtcpsock, remotertcpPort);
    
    tracef("addDataChannel channelId:%d ok\n", channelId);
    return 0;
}

int32_t TransportChannelIndepent::setDataCallback(DataProc callback) {
    for (auto &it : mTransports) {
        if (it.rtcpTransport) {
            it.rtcpTransport->setDataCallback(callback);
        }
    }
    return true;
}

int32_t TransportChannelIndepent::setExceptionCallback(ExceptionCallBack callback) {
    for (auto &it : mTransports) {
        if (it.rtcpTransport) {
            it.rtcpTransport->setExceptionCallback(callback);
        }
    }
    return true;
}

bool TransportChannelIndepent::start() {
    for (auto &it : mTransports) {
        if (it.rtcpTransport) {
            it.rtcpTransport->startReceive();
        }
    }
    return true;
}

bool TransportChannelIndepent::stop() {
    for (auto &it : mTransports) {
        if (it.rtcpTransport) {
            it.rtcpTransport->stopReceive();
        }
    }
    return true;
}

int32_t TransportChannelIndepent::sendMedia(int32_t channelId, const MediaFrame &frame, int32_t mark) {
    if (channelId > 5) {
        //tracef("channelId:%d, sendMedia len:%d\n", channelId, frame.size());
    }
    //tracef("channelId:%d, sendMedia len:%d\n", channelId, frame.size());

    if (!mTransports[channelId].rtpTransport) {
        errorf("channel:%d transport is nullptr\n", channelId);
        return -1;
    }

    int32_t sendLen = 0;
    int32_t dataLen = frame.size();
    char *buffer = (char*)frame.data();
    int32_t packLen = 0;
    struct RtpTcpHdr *hdr;
    int32_t tryCount = 0;
    int32_t send_count = 0;
    int32_t send_count_max = 50;
    do {

        hdr = (struct RtpTcpHdr*)(buffer + sendLen);
        packLen = ntohs(hdr->len);

        int32_t ret = mTransports[channelId].rtpTransport->send((const char*)(buffer + sendLen + sizeof(struct RtpTcpHdr)), packLen);
        if (ret < 0) {
            errorf("TransportChannelIndepent send rtp failed, ret:%d\n", ret);
            return ret;
        } else if ((ret == 0) || (ret != packLen)) {
            tryCount++;
            if (tryCount > 10) {
                errorf("udp send tryCount %d\n", tryCount);
                return -1;
            }
            //停一会儿再发，重发这个包
            //warnf("udp send packLen:%d, ret:%d, tryCount:%d\n", packLen, ret, tryCount);
            ret = 0; 
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {

            tryCount = 0;
            sendLen += (ret + sizeof(struct RtpTcpHdr));

            /* 简单流控 */
            send_count++;
            if (send_count > send_count_max) {
                send_count = 0;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        if (channelId < trackMax) {
            mSentBytes[channelId] += ret;
            if (ret > 0) {
                mSentPackets[channelId] += 1;
            }
        }

    } while(sendLen < dataLen);

    return sendLen;
}

int32_t TransportChannelIndepent::sendRtcp(int32_t channelId, const MediaFrame &frame) {
    if (!mTransports[channelId].rtpTransport) {
        errorf("channel:%d transport is nullptr\n", channelId);
        return -1;
    }
    
    char *buffer = (char*)frame.data();
    struct RtpTcpHdr *hdr = (struct RtpTcpHdr*)(buffer);
    int32_t dataLen = ntohs(hdr->len);
    int32_t ret = mTransports[channelId].rtcpTransport->send((const char*)(buffer + sizeof(struct RtpTcpHdr)), dataLen);
    if (ret < 0) {
        errorf("TransportChannelIndepent send rtcp failed, ret:%d\n", ret);
        return ret;
    }
    return ret;
}

void TransportChannelIndepent::getSendStatistic(int32_t channelId, uint32_t &sendPackets, uint32_t &sendBytes){
    sendBytes = mSentBytes[channelId];
    sendPackets = mSentPackets[channelId];
}
