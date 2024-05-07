/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtcpParser.h
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:43:14
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <list>
#include <memory>
#include <string>
#include "RtspUtils.h"

class RtcpParser {
public:
#pragma pack(push)
#pragma pack(1)
    struct rtcp_hdr_t {
        uint8_t flag;
        uint8_t pt;
        uint16_t length;
        //uint32_t ssrc;
    };

    /// RTCP 载荷类型
    enum {
        RTCP_TYPE_SR   = 200,   /// RTCP sender report
        RTCP_TYPE_RR   = 201,   /// RTCP receiver report
        RTCP_TYPE_SDES = 202,   /// RTCP source description packet
        RTCP_TYPE_BYE  = 203,   /// RTCP bye packet
        RTCP_TYPE_APP  = 204    /// RTCP packet containing application specific data
    };

    /// RR 接收者报告块
    struct  rr_block_t {
        uint32_t ssrc;            /* data source being reported */
        uint32_t fraction:8,      /* fraction lost since last SR/RR */
        lost:24;                  /* cumul. no. pkts lost (signed!) */
        uint32_t last_seq;        /* extended last seq. no. received */
        uint32_t jitter;          /* interarrival jitter */
        uint32_t lsr;             /* last SR packet from this source */
        uint32_t dlsr;            /* delay since last SR packet */
    };
#pragma pack(pop)

    /// 源描述子项类型
    enum {
        END = 0,     /* Used when the iteration over the items has finished. */
        CNAME,       /* Used for a CNAME (canonical name) item. */
        NAME,        /* Used for a NAME item. */
        EMAIL,       /* Used for an EMAIL item. */
        PHONE,       /* Used for a PHONE item. */
        LOC,         /* Used for a LOC (location) item. */
        TOOL,        /* Used for a TOOL item. */
        NOTE,        /* Used for a NOTE item. */
        PRIV,        /* Used for a PRIV item. */
        Unknown      /* Used when there is an item present, but the type is not recognized. */
    };

    /// 源描述子项
    struct  sdes_item_t {
        uint8_t type;        /* type of item (rtcp_sdes_type_t) */
        uint8_t length;      /* length of item (in octets) */
        uint8_t data[0];     /* text, not null-terminated */
    };

    typedef std::list< sdes_item_t* > ItemList;

    /// SDES包
    struct rtcp_sdes_t {
        uint32_t    ssrc;          /* first SSRC/CSRC */
        ItemList    *item_list;    /* list of SDES items */
    };

    /// SR包
    struct rtcp_sr_t {
        uint32_t ssrc;        /* sender generating this report */
        uint32_t ntp_sec;     /* NTP timestamp */
        uint32_t ntp_frac;    /* for less than 68 years, padding with 0 */
        uint32_t rtp_ts;      /* RTP timestamp */
        uint32_t psent;       /* packets sent */
        uint32_t osent;       /* octets sent */
    };

    /// BYE包
    struct rtcp_bye_t {
        uint32_t    ssrc;             /* sender generating this report */
        uint8_t     reason[0];        /* reason data, optional support */
    };

    /// RTCP包结构体
    struct rtcp_pack_t {
        struct rtcp_hdr_t common;     /* 通用头部信息 */
        union {                       /* 经过内部解析之后，填充数据包的详细信息 */
            struct rtcp_sr_t sr;      /* sender report (SR) */
            struct rr_block_t rr;     /* reception report (RR) */
            struct rtcp_sdes_t sdes;  /* source description (SDES) */
            struct rtcp_bye_t bye;    /* BYE */
        } pack;
    };

    //发送者统计信息
    struct sr_statistic_t {
        uint32_t  send_packets;        /* 总共发送的RTP包个数 */
        uint32_t  send_bytes;          /* 总共发送的RTP字节数  */
        uint32_t  send_rtp_pts;        /* 最新发送的RTP包时间戳 */
    };

public:

    RtcpParser(uint32_t ssrc);
    ~RtcpParser();

    std::list<std::shared_ptr<rtcp_pack_t>> parse(const char* request, int32_t len, bool &isBye, int32_t &used);

    int32_t getSRPacket(uint8_t *buffer, int32_t length);

    int32_t addSDESItem(uint8_t type, uint8_t length, uint8_t *data);

    void updateInfo(struct sr_statistic_t &sr_statistic);

private:

    uint64_t getNptTime1900();

    bool isSpaceEnough(uint32_t buflen, uint32_t datalen, uint16_t& padding);

    int32_t buildSRPack(uint8_t *buf, uint32_t length);

    int32_t buildSdesPack(uint8_t *buf, uint32_t length);

private:

    uint32_t                ssrc_;
    struct sr_statistic_t   SRStatistic_;      ///发送者统计信息
    struct rtcp_sdes_t*     sdes_;             ///用户添加的源描述信息
};
