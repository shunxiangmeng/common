/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtcpParser.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-03-31 21:45:15
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <list>
#include <string>
#include <string.h>
#include "RtspUtils.h"
#include "RtcpParser.h"
#include "infra/include/Logger.h"
#include "infra/include/Timestamp.h"
#include "infra/include/ByteIO.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif // _WIN32

RtcpParser::RtcpParser(uint32_t ssrc) : ssrc_(ssrc), sdes_(nullptr) {
    memset(&SRStatistic_, 0, sizeof(struct sr_statistic_t));
    char name[32] = "IPC";
    addSDESItem(CNAME, (uint8_t)strlen(name), (uint8_t *)name);
}

RtcpParser::~RtcpParser() {
}

std::list<std::shared_ptr<RtcpParser::rtcp_pack_t>> RtcpParser::parse(const char* data, int32_t length, bool &isBye, int32_t &used) {
    isBye = false;
    used = 0;
    std::list<std::shared_ptr<rtcp_pack_t>> rtcp_packet_lists;
    if (data == nullptr || (unsigned)length < sizeof(rtcp_hdr_t)){
        return rtcp_packet_lists;
    }

    int32_t pos = 0;
    int32_t packNum = 0;
    while (pos < length) {
        rtcp_hdr_t *rtcphdr = (rtcp_hdr_t*)(data + pos);
        int32_t version = rtcphdr->flag >> 6;
        int32_t p = rtcphdr->flag & 0x20;
        int32_t count = rtcphdr->flag & 0x1f;
        if (version != 2 || p != 0 || rtcphdr->pt < RTCP_TYPE_SR) {
            //warnf("invalid rtcp packet, rtcp count:%d, version:%d, p:%d, pt:%d\n", count, version, p, rtcphdr->pt);
            used = length;
            return rtcp_packet_lists;
        }

        int32_t packLength = ntohs(rtcphdr->length) * 4;
        if (pos + packLength > length) {
            errorf("rtcp packet length error, expect:%d, actual:%d\n", pos + packLength, length);
            used = length;
            return rtcp_packet_lists;
        }

        if (rtcphdr->pt == RTCP_TYPE_BYE) {
            isBye = true;
            break;
        } else if (rtcphdr->pt == RTCP_TYPE_RR) {
            if (packLength < sizeof(rr_block_t)) {
            } else {
                std::shared_ptr<rtcp_pack_t> rtcp_pack = std::make_shared<rtcp_pack_t>();
                memcpy(&rtcp_pack->common, rtcphdr, sizeof(rtcp_hdr_t));
                const uint8_t* p = (const uint8_t*)(data + pos + sizeof(rtcp_hdr_t));
                rtcp_pack->pack.rr.ssrc = infra::ByteReader<uint32_t>::ReadBigEndian(&p[0]);
                rtcp_pack->pack.rr.fraction = p[4];
                rtcp_pack->pack.rr.lost = infra::ByteReader<int32_t, 3>::ReadBigEndian(&p[5]);
                rtcp_pack->pack.rr.last_seq = infra::ByteReader<uint32_t>::ReadBigEndian(&p[8]);
                rtcp_pack->pack.rr.jitter = infra::ByteReader<uint32_t>::ReadBigEndian(&p[12]);
                rtcp_pack->pack.rr.lsr = infra::ByteReader<uint32_t>::ReadBigEndian(&p[16]);
                rtcp_pack->pack.rr.dlsr = infra::ByteReader<uint32_t>::ReadBigEndian(&p[20]);
                rtcp_packet_lists.push_back(rtcp_pack);
            }
        } else {
            tracef("not impl rtcp block type:%d\n", rtcphdr->pt);
        }
        pos += packLength;
        packNum++;
    }

    used = pos;
    return rtcp_packet_lists;
}

bool RtcpParser::isSpaceEnough(uint32_t buflen, uint32_t datalen, uint16_t& padding){
    bool ret = false;
    uint16_t tmp_padding = ( 4 - datalen % 4 ) % 4;
    datalen += tmp_padding;
    if (buflen < datalen) {
        return ret;
    }
    ret = true;
    padding = tmp_padding;
    return ret;
}

/**
 * #define MILLISEC1900 ( (uint64_t)1000 * (24 * 60 * 60) * (uint64_t)((70 * 365) + 17))
 */

uint64_t RtcpParser::getNptTime1900() {
    uint64_t curtime = 0;
    curtime = infra::getCurrentTimeMs();
    //curtime -= 1000*g_TimeZone[mm_time_zone].iSecond; //lint !e647
    return curtime;
}

//    Sender report (SR) (RFC 3550).
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |V=2|P|    RC   |   PT=SR=200   |             length            |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |                         SSRC of sender                        |
//    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  4 |              NTP timestamp, most significant word             |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |             NTP timestamp, least significant word             |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 12 |                         RTP timestamp                         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                     sender's packet count                     |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                      sender's octet count                     |
// 24 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

int32_t RtcpParser::buildSRPack(uint8_t *buf, uint32_t length) {
    if (length < sizeof(rtcp_hdr_t) + sizeof(rtcp_sr_t)) {
        return -1;
    }

    rtcp_hdr_t *header = (rtcp_hdr_t*)buf;
    header->flag = 0;            ///RC=0
    header->flag |= 0x02 << 6;   ///version
    header->flag |= 0x00 << 5;   ///padding
    header->pt = RTCP_TYPE_SR;
    uint16_t len = sizeof(rtcp_hdr_t);

    rtcp_sr_t *sr = (rtcp_sr_t*)(buf + len);
    sr->ssrc     = htons(ssrc_);
    uint64_t npt = getNptTime1900();
    sr->ntp_sec  = htonl((uint32_t)(npt >> 32));
    sr->ntp_frac = htonl((uint32_t)npt );
    sr->rtp_ts   = htonl(SRStatistic_.send_rtp_pts);
    sr->psent    = htonl(SRStatistic_.send_packets);
    sr->osent    = htonl(SRStatistic_.send_bytes);

    len += sizeof(struct rtcp_sr_t);
    header->length = htons(6);
    return len;
}

int32_t RtcpParser::buildSdesPack(uint8_t *buf, uint32_t length){
    if (length < sizeof(rtcp_hdr_t) + sizeof(uint32_t) + 4){
        return -1;    
    }

    rtcp_hdr_t *header = (rtcp_hdr_t*)buf;
    header->flag = 0;
    header->flag |= 0x02 << 6;   ///version
    header->flag |= 0x00 << 5;   ///padding
    header->pt = RTCP_TYPE_SDES;
    uint16_t len = sizeof(struct rtcp_hdr_t); 

    rtcp_sdes_t *rtcp_sdes_dst = (rtcp_sdes_t *)( buf + len );
    rtcp_sdes_dst->ssrc = htonl(sdes_->ssrc);
    len += sizeof(uint32_t);
    uint16_t padding = 3;
    struct sdes_item_t *item_dst = (struct sdes_item_t *)( buf + len);

    for (auto item = sdes_->item_list->begin(); item != sdes_->item_list->end(); item ++) {
        if (!isSpaceEnough(length, len + (*item)->length + sizeof(sdes_item_t) + 1, padding)) {
            break;
        }

        item_dst->type = (*item)->type;
        item_dst->length = (*item)->length;
        memcpy(item_dst->data, (*item)->data, (*item)->length);
        len += item_dst->length + sizeof(sdes_item_t);
        item_dst = (struct sdes_item_t *)(buf + len); 
    }

    buf[len++] = END;

    if (padding > 0) {
        memset( buf + len, '\0', padding );
        len += padding;
    }

    header->length = htons( len/4 - 1 );
    return len;
}

int32_t RtcpParser::getSRPacket(uint8_t *buffer, int32_t length) {
    if (buffer == nullptr || (unsigned)length < 2 * sizeof(rtcp_hdr_t) + sizeof(rtcp_sr_t)) {
        return -1;
    }

    int32_t srlen = buildSRPack(buffer, length);
    if (srlen < 0) {
        errorf("buildSRPack error\n");
        return -1;
    }

    /*int32_t sdeslen = buildSdesPack(buffer + srlen, length - srlen);
    if (sdeslen < 0) {
        errorf("buildSdesPack error\n");
        return -1;
    }*/

    return srlen;
}

int32_t RtcpParser::addSDESItem(uint8_t type, uint8_t length, uint8_t *data) {
    if (!data || length <= 0) {
        return -1;
    }

    /* 添加ssrc 信息 */
    if (!sdes_) {
        sdes_ = new rtcp_sdes_t;
        sdes_->ssrc = ssrc_;
        sdes_->item_list = new ItemList;
    }

    struct sdes_item_t *new_sdes_item = (struct sdes_item_t *)calloc(1, sizeof(struct sdes_item_t) + length);
    if (nullptr == new_sdes_item) {
        /* m_sdes, 和 m_sdes->item_list 会在析构时释放 */
        errorf("calloc failed.\n");
        return -1;
    }

    new_sdes_item->type = type;
    new_sdes_item->length = length;
    memcpy(new_sdes_item->data, data, length);
    sdes_->item_list->push_back(new_sdes_item);    
    return 0; 
}

void RtcpParser::updateInfo(struct sr_statistic_t &sr_statistic) {
    SRStatistic_ = sr_statistic;
}
