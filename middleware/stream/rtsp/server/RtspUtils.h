/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspUtils.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:54:35
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once

///\brief 系统所支持的全部Rtsp方法
enum RtspMethod {
    rtspMethodOptions,      ///> OPTION命令,支持做保活信令
    rtspMethodDescribe,     ///> DESCRIBE命令
    rtspMethodAnnounce,     ///> ANNOUNCE命令
    rtspMethodSetup,        ///> SETUP命令
    rtspMethodPlay,         ///> PLAY命令    
    rtspMethodRecord,       ///> RECORD命令
    rtspMethodPause,        ///> PAUSE命令
    rtspMethodTeardown,     ///> TEARDOWN命令
    rtspMethodSetParameter, ///> SET_PARAMETER命令
    rtspMethodGetParameter, ///> GET_PARAMETER命令，支持做保活信令
    rtspMethodMax,          ///> 未知命令
};

///\brief 时间/位置  间隔类型定义
enum RangeType {
    rangeTypeNpt,        ///> NPT时间类型
    rangeTypeClock,      ///> Clock时间类型
    rangeTypeByte,       ///> 字节位置偏移信息
    rangeTypeSmpte,      ///> Smpte时间格式
    rangeTypeSlice,      ///> 分片偏移信息, 主要HLS 协议使用
    rangeTypeUnknow,     ///> 未知格式
};

///\brief UTC时间格式
struct UtcTime {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t minute;
    int32_t second;
    int32_t millisec;
};

///\brief SMPTE格式定义
struct SmpteTime {
    int32_t smpteTimeHour;      ///> 相对文件的相对小时
    int32_t smpteTimeMinute;    ///> 相对文件的相对分钟
    int32_t smpteTimeSecond;    ///> 相对文件的相对秒
};

///\brief 位置/时间 间隔信息
struct RangeInfo {
    RangeType rangeType;    ///> 间隔格式
    union {
        struct npt_t {
            double start;
            double end;
        } npt;

        struct utc_t {
            struct UtcTime start;
            struct UtcTime end;
        } utc;

        struct byte_t {
            int64_t start;
            int64_t end;
        } byte;

        struct smpte_t {
            struct SmpteTime start;
            struct SmpteTime end;
        } smpte;

        struct slice_t {
            int32_t start;
            int32_t end;
        }slice;
    } rangeInfo;     ///> 间隔信息
};

///\brief Rtsp请求RTP传输时采用的传输层协议
enum RtpProtocol {
    rtpProtocolRtpOverRtsp,    ///> 交织方式
    rtpProtocolRtpOverUdp,     ///> UDP单播方式
    rtpProtocolTranportAuto,   ///> 自动传输
    rtpProtocolMulticast,      ///> 组播方式
    rtpProtocolNum,            ///> 未知格式
};

struct ResponseText {
    uint32_t        code;        ///> RTSP响应码
    const char     *textInfo;    ///> 响应信息
};

enum StreamSourceType {
    StreamSourceTypeLive,     ///直播
    StreamSourceTypeVod,      ///点播
};
