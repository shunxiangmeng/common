/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspSession.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 23:13:40
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "RtspSession.h"
#include "infra/include/Logger.h"
#include "jsoncpp/include/json.h"
#include "infra/include/network/Defines.h"
#include "infra/include/thread/WorkThreadPool.h"

RtspSession::RtspSession(IRtspSessionManager* manager):
    session_manager_(manager), 
    send_exception_(false), destroied_(false) {
    tracef("new rtsp session %p\n", this);
}

RtspSession::~RtspSession() {
    tracef("~RtspSession\n");
}

bool RtspSession::init(std::shared_ptr<infra::TcpSocket> &newSock, const char *buffer, int32_t len) {
    if (!newSock || buffer == nullptr) {
        return false;
    }
    std::string remote_ip = newSock->getRemoteIp();
    tracef("rtsp remote ip %s\n", remote_ip.data());

    sock_ = newSock;

    if (!cmd_transport_) {
        cmd_transport_ = std::make_shared<TransportChannelInterleave>();
    }

    cmd_transport_->setChannelSock(sock_);
    cmd_transport_->setDataCallback(DataProc(&RtspSession::onMessage, this));
    cmd_transport_->setExceptionCallback(ExceptionCallBack(&RtspSession::onException, this));

    if (parseAndProcess(buffer, len) == 0) {
        return false;
    }

    cmd_transport_->start();
    return true;
}

int32_t RtspSession::onMessage(int32_t channel, const char* data, int32_t dataLen) {
    if (data[0] == '$') {
        if (dataLen <= 4) {
            return 0;
        }
    } else {
        infof("onMessage len:%d\n%s\n", dataLen, data);
    }

    int32_t used = parseAndProcess(data, dataLen);
    return used;
}

void RtspSession::onException(int32_t code) {
    warnf("onException code:%d\n", code);
    destroySession();
}

int32_t RtspSession::sendCommand(const char* cmd, int32_t len) { 
    tracef("response len:%d\n%s", len, cmd);
    return cmd_transport_->sendCommand(cmd, len);
}

int32_t RtspSession::startStreaming() {
    if (!media_session_ || !media_session_->start(MediaSession::OnFrameProc(&RtspSession::onMediaData, this))) {
        errorf("startStreaming failed\n");
        return -1;
    }
    return 0;
}

int32_t RtspSession::stopStreaming() {
    if (!media_session_ || !media_session_->stop(MediaSession::OnFrameProc(&RtspSession::onMediaData, this))) {
        errorf("stopStreaming failed\n");
        return -1;
    }
    return 0;
}

void RtspSession::onMediaData(MediaFrameType type, MediaFrame &frame) {
    //tracef("onMediaData index:%d, size:%d\n", int32_t(type), frame.size());
    if (send_exception_) {
        return;
    }

    TrackIndex index;
    if (type == Video) {
        index = trackVideo;
    } else if (type == Audio) {
        index = trackAudio;
    } else {
        errorf("rtsp not support MediaFrameType %d\n", type);
        return;
    }

    if (rtp_packager_->putPacket(index, frame) > 0) {
        MediaFrame rtp_frame;
        if (rtp_packager_->getPacket(index, rtp_frame) > 0) {
            int32_t ret = 0;
            switch (transport_info_[index].transType) {
                case rtpProtocolRtpOverRtsp:
                    ret = cmd_transport_->sendMedia(index, rtp_frame, 0);
                    break;
                case rtpProtocolRtpOverUdp:
                    ret = udp_transport_->sendMedia(index, rtp_frame, 0);
                    break;
                default:
                    break;
            }

            if (ret < 0) {
                warnf("send failed, close connection\n");
                send_exception_ = true;
                onMediaDataDestroySession();
            }
        }
    }
}

int32_t RtspSession::sendRtcp() {
    uint8_t buffer[1400] = {0};
    for (auto i = 0; i < trackMax; i++) {
        RTP::TrackInfo info;
        if (!rtp_packager_  || !rtp_packager_->getTrackInfo(TrackIndex(i), info)) {
            continue;
        }
        if (transport_info_[i].on == false) {
            continue;
        }

        memset(buffer, 0x00, sizeof(buffer));
        uint32_t sentPackets = 0, sentBytes = 0;

        if (transport_info_[i].transType == rtpProtocolRtpOverRtsp) {
            cmd_transport_->getSendStatistic(i, sentPackets, sentBytes);
        } else if (transport_info_[i].transType == rtpProtocolRtpOverUdp) {
            udp_transport_->getSendStatistic(i, sentPackets, sentBytes);
        }

        struct RtcpParser::sr_statistic_t srStatistic = {0};
        srStatistic.send_rtp_pts = info.timestamp;
        srStatistic.send_packets = sentPackets;
        srStatistic.send_bytes = sentBytes;
        if (!rtcp_parser_[i].get()) {
            continue;
        }
        rtcp_parser_[i]->updateInfo(srStatistic);

        int32_t rtcplen = rtcp_parser_[i]->getSRPacket(buffer + sizeof(struct RtpTcpHdr), sizeof(buffer) - sizeof(struct RtpTcpHdr));
        if (rtcplen <= 0) {
            continue;
        }

        MediaFrame rtcpFrame(rtcplen + sizeof(struct RtpTcpHdr));
        if (rtcpFrame.empty()) {
            break;
        }

        rtcpFrame.setSize(0);
        struct RtpTcpHdr *tcphdr = (struct RtpTcpHdr*)buffer;
        tcphdr->dollar = '$';
        tcphdr->channel = i * 2 + 1;
        tcphdr->len = htons(rtcplen);
        rtcpFrame.putData((const char*)buffer, rtcplen + sizeof(struct RtpTcpHdr));

        if (transport_info_[i].transType == rtpProtocolRtpOverRtsp) {
            cmd_transport_->sendMedia(i, rtcpFrame, 0);
        } else if (transport_info_[i].transType == rtpProtocolRtpOverUdp) {
            //int32_t rtcpChn = transport_info_[i].rtcpChannelId;
            udp_transport_->sendRtcp(i, rtcpFrame);
        }
    }

    return 0;
}

void RtspSession::destroySession() {
    tracef("destroySession\n");
    if (destroied_) {
        infof("RtspSession has destroied\n");
        return;
    }
    destroied_ = true;
    stopStreaming();
    RtspSessionBase::destroySession();
    infra::WorkThreadPool::instance()->async([&] () {
        if (cmd_transport_) {
            if (!cmd_transport_->stop()) {
                errorf("stop failed\n");
            }
        }
        if (udp_transport_) {
            udp_transport_->stop();
        }
        session_manager_->remove(shared_from_this());
    });
}

void RtspSession::onMediaDataDestroySession() {
    tracef("onMediaDataDestroySession\n");
    destroySession();
}
