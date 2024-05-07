/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSessionBase.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 23:26:59
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "MediaInfo.h"
#include "RtcpParser.h"
#include "SessionTimer.h"
#include "RtspMessageParser.h"
#include "stream/mediasession/MediaSession.h"
#include "infra/include/network/Socket.h"
#include "infra/include/network/TcpSocket.h"
#include "rtsp/common/Defines.h"
#include "rtp/RTPPack.h"
#include "Transport/TransportChannelInterleave.h"
#include "Transport/TransportChannelIndepent.h"

#define RTCP_INTERVEAD 2

struct TCPHDR {
    uint8_t  dollor;
    uint8_t  channel;
    uint16_t size;
};

enum StreamEvent {
    streamEventSdpChange,        /// 流源发生变更
    streamEventReceiveRtcpSR,    /// 收到RTCP发送报告
    streamEventReceiveRtcpRR,    /// 收到RTCP接收报告
    streamEventReceiveRtcpBYE,   /// 收到rtcp bye报文
};

struct StreamEventParameter {
    int32_t mediaIndex;
};

class RtspSessionBase {
public:

    RtspSessionBase();

    ~RtspSessionBase();

    int32_t parseAndProcess(const char *buffer, int32_t len);

    int32_t dealRequest(RtspMessage &message);

    virtual void destroySession();

protected:

    void sendResponse(RtspMessage &message, uint32_t status_code, const std::string &content = std::string());

    virtual int32_t sendCommand(const char* cmd, int32_t len) { return -1; }

    virtual int32_t startStreaming() = 0;

    virtual int32_t stopStreaming() = 0;

    virtual int32_t updateTransport(int32_t mediaIndex, int32_t &rtpport, int32_t &rtcpport);

    virtual int32_t sendRtcp() = 0;

    int32_t onStreamEvent(StreamEvent event, const StreamEventParameter& parameter);

private:
    int32_t dealDescribe(RtspMessage &message);
    int32_t dealAnnounce(RtspMessage &message);
    int32_t dealSetup(RtspMessage &message);
    int32_t dealPlay(RtspMessage &message);
    int32_t dealPause(RtspMessage &message);
    int32_t dealRecord(RtspMessage &message);
    int32_t dealTeardown(RtspMessage &message);
    int32_t dealSetParameter(RtspMessage &message);
    int32_t dealGetParameter(RtspMessage &message);

    bool parseUrl(std::string &url);
    std::string makeSdp();
    int32_t tryOpenUdpRtpRtcpSocket(std::shared_ptr<infra::Socket>& rtpsock, std::shared_ptr<infra::Socket>& rtcpsock);
    void aliveTimeout();
    void sendRtcpTimeout();
    int32_t onRtcpMessage(int32_t channel, const char* data, int32_t dataLen);

protected: 

    std::shared_ptr<infra::TcpSocket> sock_;

    struct TransportInfo {
        bool                on;
        RtpProtocol         transType;
        int32_t             rtpChannelId;
        int32_t             rtcpChannelId;
        uint32_t            ssrc;

        union {
            struct Tcp {
                int32_t     rtpInterleavedChannel;
                int32_t     rtcpInterleavedChannel;
            } tcp;

            struct Udp {
                char        remoteIp[32];
                int32_t     localRtpSockFd;
                int32_t     localRtcpSockFd;
                int32_t     remoteRtpPort;
                int32_t     remoteRtcpPort;
            } udp;
        } option;
    };

    TransportInfo                      transport_info_[TrackIndex::trackMax];
    std::shared_ptr<MediaSession>      media_session_;
    std::shared_ptr<RtcpParser>        rtcp_parser_[TrackIndex::trackMax];

    std::shared_ptr<TransportChannelInterleave> cmd_transport_;
    std::shared_ptr<TransportChannelIndepent>   udp_transport_;
    std::shared_ptr<RTP::RTPPack>  rtp_packager_;

private:

    bool                               session_start_;
    std::shared_ptr<RtspMessage>       rtsp_message_;
    std::shared_ptr<RtspMessageParser> msg_parser_;

    struct UrlInfo{
        StreamSourceType  sourceType;
        int32_t           channel;        ///通道号，从1开始
        int32_t           streamType;     ///码流类型，从1开始
        bool              multicastAttr;  ///组播信息
        std::string       content;        ///url类容
        std::string       multiConeten;   ///组播类容
    };

    UrlInfo url_info_;

    std::shared_ptr<SessionTimer>   keep_timer_;
    std::shared_ptr<SessionTimer>   rtcp_timer_;
};
