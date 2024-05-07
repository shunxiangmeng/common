/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtpPack.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-05 12:11:57
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include "common/mediaframe/MediaFrame.h"
#include "rtsp/common/Defines.h"
#include "jsoncpp/include/value.h"
#include "RTPCut.h"

namespace RTP {

typedef struct {
    bool            on;                /* 是否开启 */
    int32_t         interleave;        /* 交织通道号 */
    uint32_t        ssrc;              /* ssrc */
    uint32_t        timestamp;         /* 当前包的timestamp */
    uint32_t        sequence;          /* 当前包的sequence */
    uint32_t        totalSize;         /* 总共的数据量 */
    uint32_t        totalPackets;      /* 总包数 */
    int32_t         freq;              /* 采样频率 */
    int32_t         payload;           /* 负载 */
} TrackInfo;

class RTPPack {
    RTPPack(const RTPPack &other) = delete;
    RTPPack& operator= (const RTPPack &other) = delete;
public:
    /**
     * @brief 构造
     * @param[in] src
     */
    RTPPack();
    /**
     * @brief 析构
     */
	virtual ~RTPPack();

	typedef struct {
	    int8_t      overTcp;
        int32_t     packLen;
        TrackInfo   trackInfo[trackMax];
	} RTPOption;

public:
    /**
     * @brief 输入一帧或一包，用于转码
     * @param[in] index      媒体的索引号 video - trackVideo, audio - trackAudio
     * @param[in] frame      待转码的数据包
     * @return 1-转码完成, 可以调用getPacket提取数据,0-转码未完成，数据不足，-1-错误
     */
    virtual int32_t putPacket(TrackIndex index, MediaFrame &frame);
    /**
     * @brief 获取转码好的数据
     * @param[in] index 媒体的索引号
     * @param[in] frame 已转码的数据包
     */
    virtual int32_t getPacket(TrackIndex index, MediaFrame &frame);
    /**
     * @brief 设置转码通道参数
     * @param[in] option  转码参数
     * @return true-成功，false-失败
     */
    virtual bool setOption(const Json::Value& option);
    virtual bool setOption(const RTPOption& option);
    /**
     * @brief 设置转码通道参数
     * @param[out] option 转码参数
     * @return
     */
    virtual void getOption(Json::Value& option);
    virtual bool getOption(RTPOption& option);
    /**
     * @brief 设置转码通道的打包参数
     * @param[in] index
     * @param[in] info
     * @return true-成功，false-失败
     */
    virtual bool setTrackInfo(TrackIndex index, const Json::Value& info);
    virtual bool setTrackInfo(TrackIndex index, const TrackInfo& info);
    /**
     * @brief 获取转码通道的打包参数
     * @param[in] index
     * @param[in] info
     * @return true-成功，false-失败
     */
    virtual bool getTrackInfo(TrackIndex index, Json::Value& info);
    virtual bool getTrackInfo(TrackIndex index, TrackInfo& info);

private:
    /**
     * @brief 构建mediaFrame
     * @param[in]  mediaIndex
     * @param[out] frame
     */
    void makeFrame(TrackIndex mediaIndex, MediaFrame &frame);

private:
    bool         mWaitIFrame;
    RTPOption    mPackInfo;
    int32_t      mCodec[trackMax];
    uint64_t     mSyncPts[trackMax];
    MediaFrame   mOutFrame[trackMax];
    RtpPackInfo  mRtpPack[trackMax];
};

}