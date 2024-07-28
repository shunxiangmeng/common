/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSessionBase.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 23:26:51
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <math.h>
#include "RtspMessageParser.h"
#include "RtspSessionBase.h"
#include "infra/include/Timestamp.h"
#include "infra/include/Logger.h"
#include "jsoncpp/include/json.h"
#include "infra/include/network/UdpSocket.h"
#include "RtspSessionManager.h"
#include "userManager/include/ILogin.h"
#include "infra/include/MD5.h"

RtspSessionBase::RtspSessionBase(IRtspSessionManager* manager) : session_manager_(manager), session_start_(true) {
    rtsp_message_ = std::shared_ptr<RtspMessage>(new RtspMessage());
    msg_parser_ = std::shared_ptr<RtspMessageParser>(new RtspMessageParser());
    memset(&transport_info_, 0x00, sizeof(transport_info_));
    for (auto &it : transport_info_) {
        it.on = false;
        it.transType = rtpProtocolNum;
    }
    rtp_packager_ = std::make_shared<RTP::RTPPack>();
}

RtspSessionBase::~RtspSessionBase() {
    warnf("~RtspSessionBase()\n");
    if (keep_timer_) {
        keep_timer_->stop();
    }
    if (rtcp_timer_) {
        rtcp_timer_->stop();
    }    
}

int32_t RtspSessionBase::parseAndProcess(const char *buffer, int32_t len) {
    int32_t usedLen = 0;
    if (buffer[0] == '$') {
        struct TCPHDR hdr = {0};
        memcpy(&hdr, buffer, sizeof(struct TCPHDR));
        hdr.size = ntohs(hdr.size);
        if ((unsigned)len < sizeof(struct TCPHDR) + hdr.size) {
            return 0;  ///不够一帧，不处理
        }

        //tracef("tcphdr len:%d\n", hdr.size);
        bool bye = false;
        int32_t track = hdr.channel / 2;
        if (rtcp_parser_[track].get()) {
            rtcp_parser_[track]->parse(buffer + sizeof(struct TCPHDR), len - sizeof(struct TCPHDR), bye, usedLen);
        }

        if (bye) {
            warnf("rtcp bye\n");
        } else if (keep_timer_) {
            keep_timer_->reset();
        }

        usedLen += sizeof(struct TCPHDR);

        //tracef("rtcp usedlen:%d, left:%d\n", usedLen, len - usedLen);
        return usedLen;
    }

    if (msg_parser_->parseRequest(buffer, len, *rtsp_message_.get(), usedLen) < 0) {
        return usedLen;
    }

    if (!keep_timer_) {
        keep_timer_ = std::make_shared<SessionTimer>();
        rtsp_message_->common_.time_out = 30;
        keep_timer_->start(rtsp_message_->common_.time_out, [&] () {
            errorf("live timeout\n");
        });
    } else {
        keep_timer_->reset();
    } 

    if (session_start_) {
        session_start_ = false;
    }

    dealRequest(*rtsp_message_.get());
    return usedLen;
}

bool RtspSessionBase::checkAuthority(RtspMessage &message) {
    if (session_manager_->isAuthority()) {
        auto getLoginChallengeInfo = [](RtspMessage &msg) {
            ILogin::LoginChallenge login_challenge;
            ILogin::instance()->loginFirst(login_challenge);
            msg.common_.authorization.type = login_challenge.authority_type;
            msg.common_.authorization.realm = login_challenge.realm;
            msg.common_.authorization.nonce = login_challenge.nonce;
        };

        if (message.common_.authorization.type == "") {
            getLoginChallengeInfo(message);
            sendResponse(message, 401);
            return false;
        } else {
            LoginInfo login_info;
            login_info.authority_type = message.common_.authorization.type;
            login_info.username = message.common_.authorization.username;
            login_info.password = message.common_.authorization.response;
            login_info.realm = message.common_.authorization.realm;
            login_info.nonce = message.common_.authorization.nonce;

            infra::MD5 md5;
            std::string to_cal_str(std::string(RtspMessageParser::getMethodString(message.method_)) + ":" + message.common_.authorization.uri);
            md5.update(to_cal_str);
            login_info.authority_info = md5.finalHexString();

            if (!ILogin::instance()->loginAgain(login_info)) {
                warnf("rtsp authority failed\n");
                getLoginChallengeInfo(message);
                sendResponse(message, 401);
                return false;
            }
            return true;
        }
    }
    return true;
}

int32_t RtspSessionBase::dealRequest(RtspMessage &message) {
    //tracef("dealRequest+++\n");
    if (!checkAuthority(message)) {
        return 0;
    }
    RtspMethod method = message.method_;
    int32_t ret = -1;

    switch (method) {
        case rtspMethodOptions:
            sendResponse(message, 200);
            ret = 0;
            break;
        case rtspMethodDescribe:
            ret = dealDescribe(message);
            break;
        case rtspMethodAnnounce:
            ret = dealAnnounce(message);
            break;
        case rtspMethodSetup:
            ret = dealSetup(message);
            break;   
        case rtspMethodPlay:
            ret = dealPlay(message);
            break;
        case rtspMethodRecord:
            ret = dealRecord(message);
            break;    
        case rtspMethodPause:
            ret = dealPause(message);
            break;   
        case rtspMethodTeardown:
            ret = dealTeardown(message);
            break;   
        case rtspMethodSetParameter:
            ret = dealSetParameter(message);
            break;  
        case rtspMethodGetParameter:
            ret = dealGetParameter(message);
            break;                                                                         
    }
    return ret;
}

void RtspSessionBase::sendResponse(RtspMessage &message, uint32_t status_code, const std::string &content) {
    std::shared_ptr<char> res = msg_parser_->getReply(status_code, message, content);
    if (res.get() == nullptr) {
        warnf("res is nullptr\n");
        return;
    }
    sendCommand(res.get(), (int32_t)strlen(res.get()));
}

int32_t RtspSessionBase::dealDescribe(RtspMessage &message) {
    if (!parseUrl(message.common_.url)) {
        sendResponse(message, 400);
        return -1;
    }

    if (!media_session_) {
        media_session_ = std::make_shared<MediaSession>(url_info_.channel, url_info_.streamType);
    }
    ///先检查创建流是否成功
    if (!media_session_->create()) {
        errorf("createMedia channel %d streamType %d error\n", url_info_.channel, url_info_.streamType);
        sendResponse(message, 400);
        return -1;
    }

    tracef("mediasession create succ\n");
    std::string sdp = makeSdp();
    sendResponse(message, 200, sdp);
    return 0;
}

int32_t RtspSessionBase::dealAnnounce(RtspMessage &message) {
    errorf("not support announce\n");
    sendResponse(message, 551);
    return -1;
}

int32_t RtspSessionBase::dealSetup(RtspMessage &message) {
    std::string url = message.common_.url;
    int32_t trackId = -1;
    char* pos = nullptr;
    if ((pos = strstr((char*)url.c_str(), "trackID=")) != nullptr) {
        (void)sscanf(pos, "trackID=%d", &trackId);
        if (trackId < 0 || trackId >= trackMax) {
            errorf("trackID error %d\n", trackId);
            return -1;
        }
    }

    uint32_t ssrc = (uint32_t)infra::getCurrentTimeUs();
    ssrc &= 0x0000FFFF;
    //ssrc |= 0x80000000;
    message.setup_req_.transport.ssrc = ssrc;
    transport_info_[trackId].ssrc = ssrc;
    infof("trackID=%d ssrc %u\n", trackId, ssrc);

    /** 创建rtcp解析器 */
    if (rtcp_parser_[trackId].get() == nullptr) {
        rtcp_parser_[trackId] = std::shared_ptr<RtcpParser>(new RtcpParser(ssrc));
    }

    if (message.setup_req_.transport.proto == rtpProtocolRtpOverRtsp) {
        transport_info_[trackId].transType = rtpProtocolRtpOverRtsp;
        transport_info_[trackId].rtpChannelId = message.setup_req_.transport.cli_rtp_channel;
        transport_info_[trackId].rtcpChannelId = message.setup_req_.transport.cli_rtcp_channel;
        transport_info_[trackId].option.tcp.rtpInterleavedChannel = message.setup_req_.transport.cli_rtp_channel;
        transport_info_[trackId].option.tcp.rtcpInterleavedChannel = message.setup_req_.transport.cli_rtcp_channel;
    } else if (message.setup_req_.transport.proto == rtpProtocolRtpOverUdp) {
        transport_info_[trackId].transType = rtpProtocolRtpOverUdp;
        transport_info_[trackId].rtpChannelId = trackId * 2;
        transport_info_[trackId].rtcpChannelId = trackId * 2 + 1;
        transport_info_[trackId].option.udp.remoteRtpPort = message.setup_req_.transport.cli_rtp_channel;
        transport_info_[trackId].option.udp.remoteRtcpPort = message.setup_req_.transport.cli_rtcp_channel;

        int32_t rtpPort = -1, rtcpPort = -1;
        if (updateTransport(trackId, rtpPort, rtcpPort) < 0) {
            destroySession();
            sendResponse(message, 400);
            return -1;
        }
        message.setup_req_.transport.svr_rtp_channel = rtpPort;
        message.setup_req_.transport.svr_rtcp_channel = rtcpPort;
        tracef("local rtpPort:%d, rtcpPort:%d\n", rtpPort, rtcpPort);
    } else {
        errorf("not support rtpProtocol\n");
        sendResponse(message, 400);
        return -1;
    }

    ///分开setup
    transport_info_[trackId].on = true;
    tracef("trackId:%d, rtpChannel:%d, rtcpChannel:%d\n", trackId, transport_info_[trackId].rtpChannelId,
        transport_info_[trackId].rtcpChannelId);
    sendResponse(message, 200);
    return 0;
}

int32_t RtspSessionBase::dealPlay(RtspMessage &message) {
    infof("RtspSessionBase::dealPlay......................\n");
    if (media_session_ == nullptr) {
        errorf("mediasession is nullptr\n");
        sendResponse(message, 500);
        return -1;
    }
    RTP::RTPPack::RTPOption rtp_option;
    rtp_packager_->getOption(rtp_option);

    Json::Value info = Json::nullValue;
    for (int32_t trackId = 0; trackId < (int32_t)trackMax; trackId++) {
        if (transport_info_[trackId].transType ==  rtpProtocolNum) {
            break;
        }
        int32_t payload, rate, audio_channel_count;
        std::string payloadName;
        if (trackId == trackVideo) {
            MediaInfo::getVideoCodecInfo(media_session_, payload, payloadName, rate);
        } else if (trackId == trackAudio) {
            MediaInfo::getAudioCodecInfo(media_session_, payload, payloadName, rate, audio_channel_count);
        }

        rtp_option.trackInfo[trackId].payload = payload;
        rtp_option.trackInfo[trackId].freq = rate;
        rtp_option.trackInfo[trackId].interleave = transport_info_[trackId].option.tcp.rtpInterleavedChannel;
        rtp_option.trackInfo[trackId].ssrc = transport_info_[trackId].ssrc;
        rtp_option.trackInfo[trackId].on = transport_info_[trackId].on;
    }

    if (transport_info_[0].transType == rtpProtocolRtpOverRtsp) {
        rtp_option.overTcp = 1;
    } else if (transport_info_[0].transType == rtpProtocolRtpOverUdp) {
        /* rtptcp header在发送的时候拆掉 */
        rtp_option.overTcp = 1;
    }

    rtp_option.packLen = 1400;
    rtp_packager_->setOption(rtp_option);

    for (int32_t index = 0; index < (int32_t)trackMax; ++index) {
        if (!rtp_option.trackInfo[index].on) {
            continue;
        }

        struct RtspMessage::rtp_info rtpInfo;
        warnf("getTrackInfo:%d, ts:%u, seq:%u\n", index, rtp_option.trackInfo[index].timestamp, rtp_option.trackInfo[index].sequence);
        rtpInfo.mediaIndex = index;
        rtpInfo.ts = rtp_option.trackInfo[index].timestamp;
        rtpInfo.seq = rtp_option.trackInfo[index].sequence;
        message.play_req_.info_list.push_back(rtpInfo);
    }

    if (udp_transport_) {
        udp_transport_->start();
        udp_transport_->setDataCallback(DataProc(&RtspSessionBase::onRtcpMessage, this));
    }

    this->startStreaming();
    if (rtcp_timer_ == nullptr) {
        rtcp_timer_ = std::make_shared<SessionTimer>();
        rtcp_timer_->start(RTCP_INTERVEAD, [&] () {
            sendRtcpTimeout();
        });
    }

    sendResponse(message, 200);
    return 0;
}

int32_t RtspSessionBase::dealPause(RtspMessage &message) {
    errorf("not support pause\n");
    sendResponse(message, 551);
    return -1;
}

int32_t RtspSessionBase::dealRecord(RtspMessage &message) {
    errorf("not support record\n");
    sendResponse(message, 551);
    return -1;
}

int32_t RtspSessionBase::dealTeardown(RtspMessage &message) {
    infof("deal teardown\n");
    //this->stopStreaming();
    sendResponse(message, 200);
    destroySession();  //close
    return 0;
}

int32_t RtspSessionBase::dealSetParameter(RtspMessage &message) {
    errorf("not support Setparameter\n");
    sendResponse(message, 551);
    return -1;
}

int32_t RtspSessionBase::dealGetParameter(RtspMessage &message) {
    sendResponse(message, 200);
    return 0;
}

bool RtspSessionBase::parseUrl(std::string &url) {
    if (url.size() == 0 || strlen(url.c_str()) == 0) {
        errorf("url is empty\n");
        return false;
    }

    url_info_.content = url;
    if (url.find("live")) {
        url_info_.sourceType = StreamSourceTypeLive;
    } else if (url.find("vod")) {
        warnf("not support vod yet!\n");
        url_info_.sourceType = StreamSourceTypeVod;
    } else {
        errorf("missing parameter live or vod\n");
        return false;
    }

    char* pos = nullptr;
    if ((pos = strstr((char*)url.c_str(), "channel=")) == nullptr) {
        errorf("missing channel\n");
        return false;
    } else if (std::sscanf(pos, "channel=%d", &url_info_.channel) != 1) {
        errorf("channel error\n");
        return false;
    }

    if ((pos = strstr((char*)url.c_str(), "type=")) == nullptr) {
        errorf("missing type\n");
        return false;
    } else if (std::sscanf(pos, "type=%d", &url_info_.streamType) != 1) {
        errorf("type error\n");
        return false;
    }

    tracef("sourceType:%d channel: %d, streamType:%d\n", url_info_.sourceType, url_info_.channel, url_info_.streamType);
    return true;
}

std::string RtspSessionBase::makeSdp() {
    std::string sdp;
    int32_t len = 0;
    char buf[1024] = {0};

    ///version
    len = snprintf(buf+len, sizeof(buf)-len, "v=0\r\n");

    ///o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
    int64_t ts = infra::getCurrentTimeMs();

    std::string local_ip = sock_->getLocalIp();
    len += snprintf(buf+len, sizeof(buf)-len, "o=- %010llu %010llu IN IP4 %s\r\n", (long long unsigned int)ts, (long long unsigned int)ts, local_ip.data());

	///session name
    len += snprintf(buf+len, sizeof(buf)-len, "s=MediaServer\r\n");

	///email-address
    len += snprintf(buf+len, sizeof(buf)-len, "e=NONE\r\n");

    ///c=<network type> <address type> <connection address>
    std::string remote_ip = sock_->getRemoteIp();
    len += snprintf(buf+len, sizeof(buf)-len, "c=IN IP4 0.0.0.0\r\n");

    ///time,startTime-stopTime 0-0表示时间无限制长
    len += snprintf(buf+len, sizeof(buf)-len, "t=0 0\r\n");
    len += snprintf(buf+len, sizeof(buf)-len, "a=control:*\r\n");
    len += snprintf(buf+len, sizeof(buf)-len, "a=range:npt=now-\r\n");

    ///media video info
    int32_t payload;
    std::string payloadName;
    int32_t rate;
    MediaInfo::getVideoCodecInfo(media_session_, payload, payloadName, rate);
    len += snprintf(buf+len, sizeof(buf)-len, "m=video 0 RTP/AVP %d\r\n", payload);
    len += snprintf(buf+len, sizeof(buf)-len, "a=control:trackID=%d\r\n", trackVideo);
    len += snprintf(buf+len, sizeof(buf)-len, "a=rtpmap:%d %s/%d\r\n", payload, payloadName.c_str(), rate);
    len += snprintf(buf+len, sizeof(buf)-len, "a=recvonly\r\n");

    ///media audio info
    int32_t channel_count = 0;
    if (MediaInfo::getAudioCodecInfo(media_session_, payload, payloadName, rate, channel_count)) {
        len += snprintf(buf+len, sizeof(buf)-len, "m=audio 0 RTP/AVP %d\r\n", payload);
        len += snprintf(buf+len, sizeof(buf)-len, "a=control:trackID=%d\r\n", trackAudio);
        if (payloadName == "MPEG4-GENERIC" || payloadName == "mpeg4-generic") {   //AAC
            char config[32] = {0};
            MediaInfo::getAACAudioInfo(config, sizeof(config), rate, channel_count);
            len += snprintf(buf+len, sizeof(buf)-len, "a=rtpmap:%d %s/%d/%d\r\n", payload, payloadName.c_str(), rate, channel_count);
            len += snprintf(buf+len, sizeof(buf)-len, "a=fmtp:%d streamtype=%d;profile-level-id=%d;mode=%s;SizeLength=%d;IndexLength=%d;IndexDeltaLength=%d;config=%s\r\n", 
                payload, 5, 15, "AAC-hbr", 13, 3, 3, config);
        } else {
            len += snprintf(buf+len, sizeof(buf)-len, "a=rtpmap:%d %s/%d/%d\r\n", payload, payloadName.c_str(), rate, channel_count);
        }
        len += snprintf(buf+len, sizeof(buf)-len, "a=recvonly\r\n");  //server only send, client only recv
    }

    sdp += buf;
    sdp += "\r\n";
    return sdp;
}

int32_t RtspSessionBase::updateTransport(int32_t mediaIndex, int32_t &rtpport, int32_t &rtcpport) {
    /*if (transport_info_[mediaIndex].transType != rtpProtocolRtpOverUdp) {
        return 0;
    }*/
    if (!udp_transport_) {
        udp_transport_ = std::make_shared<TransportChannelIndepent>();
    }
    std::shared_ptr<infra::Socket> rtp_socket;
    std::shared_ptr<infra::Socket> rtcp_socket;
    if (tryOpenUdpRtpRtcpSocket(rtp_socket, rtcp_socket) == 0) {
        rtpport = rtp_socket->getLocalPort();
        rtcpport = rtcp_socket->getLocalPort();
        std::string remote_ip;
        uint16_t remote_port;
        sock_->getRemoteAddress(remote_ip, remote_port);
        memcpy(transport_info_[mediaIndex].option.udp.remoteIp, remote_ip.data(), remote_ip.length());
        infof("rtsp cmd channel remote addr:%s:%d\n", remote_ip.data(), remote_port);

        transport_info_[mediaIndex].option.udp.localRtpSockFd = rtp_socket->getHandle();
        transport_info_[mediaIndex].option.udp.localRtcpSockFd = rtcp_socket->getHandle();

        udp_transport_->addDataChannel(mediaIndex, rtp_socket, rtcp_socket,
            transport_info_[mediaIndex].option.udp.remoteIp, 
            transport_info_[mediaIndex].option.udp.remoteRtpPort,
            transport_info_[mediaIndex].option.udp.remoteRtcpPort, false);
    }
    return 0;
}

int32_t RtspSessionBase::tryOpenUdpRtpRtcpSocket(std::shared_ptr<infra::Socket>& rtpsock, std::shared_ptr<infra::Socket>& rtcpsock) {
    const int32_t port_range_min = 10000, port_range_max = 60000;
    //srand((uint32_t)time(NULL));
    srand((uint32_t)infra::getCurrentTimeUs());
    int32_t start_port = (rand() % (port_range_max - port_range_min)) + port_range_min;
    if (start_port % 2 != 0) {  ///转成偶数
        start_port += 1;
    }
    std::shared_ptr<infra::UdpSocket> rtp_socket = std::make_shared<infra::UdpSocket>();
    std::shared_ptr<infra::UdpSocket> rtcp_socket = std::make_shared<infra::UdpSocket>();
    while (true) {
        if (start_port > port_range_max) {
            errorf("guess local port error\n");
            return -1;
        }
        if (rtp_socket->open("0.0.0.0", start_port, false) == 0 && rtcp_socket->open("0.0.0.0", start_port + 1, false) == 0) {
            rtpsock = rtp_socket;
            rtcpsock = rtcp_socket;
            break;
        }
        start_port += 2;
    }
    return 0;
}

void RtspSessionBase::destroySession() {
    if (rtcp_timer_) {
        rtcp_timer_->stop();
    }
}

void RtspSessionBase::aliveTimeout() {
    errorf("alive timer timeout\n");
}

void RtspSessionBase::sendRtcpTimeout() {
    sendRtcp();
}

int32_t RtspSessionBase::onStreamEvent(StreamEvent event, const StreamEventParameter& parameter) {
    return 0;
}

int32_t RtspSessionBase::onRtcpMessage(int32_t channel, const char* data, int32_t size) {
    //infof("onRtcpMessage channel:%d, size:%d\n", channel, size);
    bool bye = false;
    int32_t track = channel;
    int32_t used = 0;
    if (rtcp_parser_[track]) {
        auto rtcp_packets = rtcp_parser_[track]->parse(data, size, bye, used);
        for (auto rtcp_packet : rtcp_packets) {
            if (rtcp_packet->common.pt == RtcpParser::RTCP_TYPE_RR) {
                /*infof("rtcp RR ssrc:%u, fraction:%d, lost:%d, last_seq:%d, jitter:%d, lsr:%d, dlsr:%d\n", 
                    rtcp_packet->pack.rr.ssrc,
                    rtcp_packet->pack.rr.fraction,
                    rtcp_packet->pack.rr.lost,
                    rtcp_packet->pack.rr.last_seq,
                    rtcp_packet->pack.rr.jitter,
                    rtcp_packet->pack.rr.lsr,
                    rtcp_packet->pack.rr.dlsr);
                */
            }
        }
    }
    if (bye) {
        warnf("rtcp bye\n");
    } else {
        if (keep_timer_) {
            keep_timer_->reset();
        }
    }
    return -1;
}