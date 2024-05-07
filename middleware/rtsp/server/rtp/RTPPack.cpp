/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RTPPack.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-05 12:16:48
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "RTPPack.h"
#include "RTP.h"
#include "infra/include/network/Defines.h"  // for htonl

namespace RTP {

RTPPack::RTPPack() {

    mRtpPack[trackVideo].mRtpPackNum = 0;
    mRtpPack[trackAudio].mRtpPackNum = 0;

    mPackInfo.overTcp = 1;
    mPackInfo.packLen = 1400;
    mPackInfo.trackInfo[trackVideo].on = false;
    mPackInfo.trackInfo[trackVideo].timestamp = 0;
    mPackInfo.trackInfo[trackVideo].sequence = 1;

    /** video default samle rate 90KHz */
    mPackInfo.trackInfo[trackVideo].freq = 90000;
    mPackInfo.trackInfo[trackVideo].totalPackets = 0;
    mPackInfo.trackInfo[trackVideo].totalSize = 0;

    /** audio */
    mPackInfo.trackInfo[trackAudio].on = false;
    mPackInfo.trackInfo[trackAudio].timestamp = 0;
    mPackInfo.trackInfo[trackAudio].sequence = 1;
    mPackInfo.trackInfo[trackAudio].freq = 8000;
    mPackInfo.trackInfo[trackAudio].totalPackets = 0;
    mPackInfo.trackInfo[trackAudio].totalSize = 0;

    mCodec[trackVideo] = mCodec[trackAudio] = -1;

    mSyncPts[trackAudio] = mSyncPts[trackVideo] = 0;
    mWaitIFrame = true;
}

RTPPack::~RTPPack() {
    for (auto &it : mOutFrame){
        it.reset();
    }
}

int32_t RTPPack::putPacket(TrackIndex index, MediaFrame &frame){
    auto type = frame.getMediaFrameType();
    if (mWaitIFrame) {
        if (type == Video) {
            VideoFrameInfo videoinfo;
            frame.getVideoFrameInfo(videoinfo);
            if (videoinfo.type != VideoFrame_I) {
                // skip frames before first I frame
                //tracef("skip the %d frame before first I frame\n", type);
                return 0;
            }
            mWaitIFrame = false;
        } else {
            //tracef("skip the %d audio frame before first I frame\n", type);
            return 0;
        }
    }

    uint8_t *buffer = (uint8_t*)frame.data();
    uint32_t size = frame.size();
    int32_t len = 0;
    PlacementType placement = frame.placementType();
    if (type == Video) {
        VideoFrameInfo info;
        frame.getVideoFrameInfo(info);
        switch (info.codec) {
            case H264:
                len = rtpCutH264(mRtpPack[index], buffer, size, mPackInfo.packLen, placement == AvccHvcc);
                break;
            case H265:
                len = rtpCutH265(mRtpPack[index], buffer, size, mPackInfo.packLen);
                break;
            default:
                errorf("rtp pack unsupport video codec:%d\n", info.codec);
                break;
        }
    } else if (type == Audio) {
        AudioFrameInfo info;
        frame.getAudioFrameInfo(info);
        switch (info.codec) {
            case AAC:
                len = rtpCutAAC(mRtpPack[index], buffer, size, mPackInfo.packLen);
                break;
            case G711a:
            case G711u:
                len = rtpCutG711(mRtpPack[index], buffer, size, mPackInfo.packLen);
                break;
            default:
                errorf("rtp pack unsupport audio codec:%d\n", info.codec);
                break;
        }
    }
    makeFrame(index, frame);
    return len;
}

int32_t RTPPack::getPacket(TrackIndex mediaIndex, MediaFrame &frame){
    int32_t frame_size = -1;
    if (!mOutFrame[mediaIndex].empty()) {
        frame = std::move(mOutFrame[mediaIndex]);
        frame_size = frame.size();
    } else {
        return frame_size;
    }
    return frame_size;
}

void RTPPack::makeFrame(TrackIndex mediaIndex, MediaFrame &originalFrame) {
    int32_t totalLength = 0;
    int32_t count = 0;
    for (auto it : mRtpPack[mediaIndex].mRtpPackVector) {
        totalLength += it.headLen + it.payloadLen;
        totalLength += sizeof(RtpHdr);
        totalLength += sizeof(RtpTcpHdr);
        count++;
        if (count >= mRtpPack[mediaIndex].mRtpPackNum) {
            break;
        }
    }

    MediaFrame frame(totalLength);
    if (frame.empty()) {
        errorf("get frame error\n");
        /**清空vector*/
        mRtpPack[mediaIndex].mRtpPackNum = 0;
        mRtpPack[mediaIndex].mRtpPackVector.clear();
        return;
    }
    frame.resize(0);

    RtpHdr rtpHdr = {0};
    rtpHdr.vpxcc = 0x80;  //V=2, P=0, X=0, CC=0
    rtpHdr.mpt = mPackInfo.trackInfo[mediaIndex].payload & 0x7F;  /** payload取低7位 */
    rtpHdr.sequence = 0;
    rtpHdr.timestamp = 0;
    rtpHdr.ssrc = htonl(mPackInfo.trackInfo[mediaIndex].ssrc);

    RtpTcpHdr tcphdr;
    tcphdr.dollar = '$';
    tcphdr.len = 0;

    /** 交织通道号 */
    tcphdr.channel = mPackInfo.trackInfo[mediaIndex].interleave;

    int64_t pts = originalFrame.pts();  //ms
    if (mSyncPts[mediaIndex] == 0) {
        mSyncPts[mediaIndex] = pts;
    }

    pts -= mSyncPts[mediaIndex];

    uint32_t ts = (uint32_t)(pts * mPackInfo.trackInfo[mediaIndex].freq / 1000);

    count = 0;
    for (const auto &it : mRtpPack[mediaIndex].mRtpPackVector){
        /** over tcp */
        if (mPackInfo.overTcp != 0) {
            tcphdr.len = htons(sizeof(RtpHdr) + it.headLen + (uint16_t)it.payloadLen);
            frame.putData((const char*)&tcphdr, sizeof(RtpTcpHdr));
        }

        /** rtp head */
        rtpHdr.sequence = htons(mPackInfo.trackInfo[mediaIndex].sequence);
        rtpHdr.timestamp = htonl(ts);
        if (count == mRtpPack[mediaIndex].mRtpPackNum - 1){
            rtpHdr.mpt |= 0x80;   ///last one, set mark bit
        } else {
            rtpHdr.mpt &= ~0x80;
        }

        frame.putData((const char*)&rtpHdr, sizeof(rtpHdr));

        /** payload */
        if (it.headLen > 0){
            frame.putData((const char*)it.head, it.headLen);
        }

        frame.putData((const char*)it.payload, it.payloadLen);

        /** update */
        mPackInfo.trackInfo[mediaIndex].sequence++;
        mPackInfo.trackInfo[mediaIndex].timestamp = ts;

        count++;
        if (count >= mRtpPack[mediaIndex].mRtpPackNum){
            break;
        }
    }

    /* clear vector */
    mRtpPack[mediaIndex].mRtpPackNum = 0;
    mRtpPack[mediaIndex].mRtpPackVector.clear();

    mOutFrame[mediaIndex] = frame;
}

bool RTPPack::setOption(const Json::Value& option) {
    const Json::Value& track = option.isMember("track") ? option["track"] : Json::nullValue;
    if (option.isMember("packLen")) {
        mPackInfo.packLen = option["packLen"].asInt();
    }
    if (option.isMember("overTcp")) {
        mPackInfo.overTcp = option["overTcp"].asBool()? 1:0;
    }
    for (int32_t index = 0; index < (int32_t)track.size() && index < trackMax; ++index) {
        setTrackInfo(TrackIndex(index), track[index]);
    }
    return true;
}

bool RTPPack::setOption(const RTPOption& option) {
    mPackInfo = option;
    return true;
}

void RTPPack::getOption(Json::Value& option) {
    option = Json::nullValue;
    option["packLen"] = mPackInfo.packLen;
    option["overTcp"] = mPackInfo.overTcp != 0;
    for (int32_t index = trackVideo; index < trackMax; ++index) {
        Json::Value elem = Json::nullValue;
        getTrackInfo(TrackIndex(index),elem);
        option["track"].append(elem);
    }
}

bool RTPPack::getOption(RTPOption& option) {
    option = mPackInfo;
    return true;
}

bool RTPPack::setTrackInfo(TrackIndex index, const Json::Value& info) {
    tracef("setTrackInfo index:%d, \n%s\n", index, info.toStyledString().c_str());
    if (index < 0 || index >= trackMax) {
        return false;
    }
    if (info.isMember("ssrc")) {
        mPackInfo.trackInfo[index].ssrc = info["ssrc"].asUInt();
    }
    if (info.isMember("freq")) {
        mPackInfo.trackInfo[index].freq = info["freq"].asInt();
    }
    if (info.isMember("interleave")) {
        mPackInfo.trackInfo[index].interleave = info["interleave"].asInt();
    }
    if (info.isMember("on")) {
        mPackInfo.trackInfo[index].on = info["on"].asBool();
    }
    if (info.isMember("sequence")) {
        mPackInfo.trackInfo[index].sequence = info["sequence"].asInt();
    }
    if (info.isMember("payload")) {
        mPackInfo.trackInfo[index].payload = info["payload"].asInt();
    }
    return true;
}

bool RTPPack::setTrackInfo(TrackIndex index, const TrackInfo& info) {
    mPackInfo.trackInfo[index] = info;
    return true;
}

bool RTPPack::getTrackInfo(TrackIndex index, Json::Value& info) {
    info = Json::nullValue;
    if (index < 0 || index >= trackMax) {
        return false;
    }
    info["on"]   = mPackInfo.trackInfo[index].on;
    info["ssrc"] = mPackInfo.trackInfo[index].ssrc;
    info["freq"] = mPackInfo.trackInfo[index].freq;
    info["sequence"] = mPackInfo.trackInfo[index].sequence;
    info["interleave"] = mPackInfo.trackInfo[index].interleave;
    info["timestamp"]  = mPackInfo.trackInfo[index].timestamp;
    info["payload"] = mPackInfo.trackInfo[index].payload;
    return true;
}

bool RTPPack::getTrackInfo(TrackIndex index, TrackInfo& info) {
    info = mPackInfo.trackInfo[index];
    return true;
}

}