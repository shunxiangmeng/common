/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspMessage.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-01 00:02:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <list>
#include <string>
#include "RtspUtils.h"

class RtspMessage {
public:

    struct attr {
        std::string name;
        std::string value;
    };

    typedef std::list<attr> AttrList;

    ///消息头域元素
    struct HeadFieldElement {
        std::string key;
        std::string value;      /** key - value */
        char        policy;     /** 'a' or 'r' or 'p' or 'l', 即追加, 替代, 前置, 列举*/
    };

    typedef std::list<HeadFieldElement> HeadFieldList;

    struct rtp_info {
        int         mediaIndex;
        uint32_t    ts;         ///Play回复的数据包时间戳
        uint16_t    seq;        ///Play回复的数据包序列号

        rtp_info() : mediaIndex(0), ts(0), seq(0) {}
    };

    typedef std::list<rtp_info> RtpInfoList;

    struct transport_info {
        uint32_t        ssrc;
        RtpProtocol     proto;
        bool            multicast;
        char            multicast_ip[128];
        int32_t         ip_type;
        int32_t         svr_rtp_channel;
        int32_t         svr_rtcp_channel;
        int32_t         cli_rtp_channel;      /// 对于TCP 传输, 为交织头通道号，对于UDP传输，为本端绑定端口
        int32_t         cli_rtcp_channel;     /// 对于TCP 传输, 为交织头通道号，对于UDP传输，为本端绑定端口
        int32_t         ttl;
    };

    struct content_info {
        std::string type;
        std::string value;
    };

    struct Authorization {
        std::string type;
        std::string username;
        std::string realm;
        std::string nonce;
        std::string uri;
        std::string response;
    };

    struct common {
        uint32_t        seq;
        std::string     user_agent;
        std::string     url;
        std::string     session_id;
        Authorization   authorization;
        HeadFieldList   headField;
        uint32_t        time_out;
    };

    ///--------------------------------------
    struct options_req {
        uint32_t        seq;
        HeadFieldList   headField;

        options_req() : seq(0) {}
    };

    struct options_rsp {
        uint32_t        code;
        int             method_num;
        RtspMethod      method[rtspMethodMax];
        struct UtcTime  cur_play_time;
        HeadFieldList   headField;

        options_rsp() : code(0), method_num(0) {
            memset(method, 0, sizeof(method));
            memset(&cur_play_time, 0, sizeof(cur_play_time));
        }
    };

    struct describe_req {
        uint32_t        seq;
        HeadFieldList   headField;

        describe_req() : seq(0) {}
    };

    struct describe_rsp {
        uint32_t        code;
        content_info    content;
        AttrList        attr;
        HeadFieldList   headField;

        describe_rsp() : code(0) {}
    };

    struct announce_req {
        uint32_t        seq;
        content_info    content;
        HeadFieldList   headField;

        announce_req() : seq(0) {}
    };
    
    struct announce_rsp {
        uint32_t        code;
        HeadFieldList   headField;

        announce_rsp() : code(0) {}
    };

    struct setup_req {
        uint32_t        seq;
        int32_t         index;
        std::string     value;
        std::string     mode;
        std::string     Transport;
        transport_info  transport;
        HeadFieldList   headField;
        int32_t         encrypttype;        ///> 加密类型

        setup_req() : seq(0), index(-1), value(""), mode("play"), encrypttype(-1) {
            memset(&transport, 0, sizeof(transport));
        }
    };    

    struct setup_rsp {
        uint32_t        code;
        uint32_t        seq;
        transport_info  transport;
        int             timeout_secs;
        int             index;
        HeadFieldList   headField;

        setup_rsp() : code(0), seq(0), timeout_secs(0), index(0) {
            memset(&transport, 0, sizeof(transport));
        }
    };

    struct play_req {
        uint32_t        seq;
        bool            only_i_frame;
        double          speed;
        RangeInfo       range;
        std::string     url;            ///if it is empty, then use common.url
        HeadFieldList   headField;
        RtpInfoList     info_list;

        play_req() : seq(0), only_i_frame(false), speed(0.0) {
            memset(&range, 0, sizeof(range));
        }
    };
    
    struct play_rsp{
        uint32_t        code;
        RangeInfo       range;
        RtpInfoList     info_list;
        HeadFieldList   headField;

        play_rsp() : code(0) {
            memset(&range, 0, sizeof(range)); 
        }
    };
        
    struct record_req {
        uint32_t        seq;
        RangeInfo       range;
        std::string     url;    /// if it is empty, then use common.url
        HeadFieldList   headField;

        record_req() : seq(0) {
            memset(&range, 0, sizeof(range));
        }
    };
    
    struct record_rsp {
        uint32_t        code;
        HeadFieldList   headField;

        record_rsp() : code(0) {}
    };

    struct pause_req {
        uint32_t        seq;
        HeadFieldList   headField;

        pause_req() : seq(0) {}
    };
        
    struct teardown_req {
        uint32_t        seq;
        std::string     url;
        HeadFieldList   headField;

        teardown_req() : seq(0) {}
    };
        
    struct parameter_req {
        uint32_t        seq;
        content_info    content;
        HeadFieldList   headField;

        parameter_req() : seq(0) {}
    };
    
    struct parameter_rsp {
        uint32_t        code;
        content_info    content;
        HeadFieldList   headField;

        parameter_rsp() : code(0) {}
    };

    ///公开成员变量
    struct common           common_;            ///>所有信令的公用信息
    struct options_req      options_req_;       ///>OPTION信令的请求信息
    struct announce_req     announce_req_;      ///>ANNOUNCE信令的请求信息
    struct describe_req     describe_req_;      ///>DESCRIBE信令的请求信息
    struct setup_req        setup_req_;         ///>SETUP信令的请求信息  
    struct play_req         play_req_;          ///>PLAY请求信息
    struct record_req       record_req_;        ///>RECORD请求信息
    struct pause_req        pause_req_;         ///>PAUSE请求信息
    struct teardown_req     teardown_req_;      ///>TEARDOWN请求信息
    struct parameter_req    parameter_req_;     ///>GETPARAMETER或SETPARAMETER请求信息
    uint32_t                cur_method_;        ///>纪录客户端当前正在执行的方法
    uint32_t                setup_req_times_;   ///>记录setup请求了几次

    RtspMethod              method_;

    ///成员方法

public:

    RtspMessage() { reset();}

    ~RtspMessage() {};

    /**
     * @brief 重置
     */
    void reset();
    /**
     * @brief 设置首个信令的序号
     * @param sequence
     */
    void setSequence(int sequence) {m_sequence = sequence;};
    /**
     * @brief 获取sequence
     * @return
     */
    uint32_t getSequence() { return m_sequence++; }

private:

    uint32_t m_sequence;        ///> CSeq序列号
};

inline void RtspMessage::reset() {

    m_sequence = 1; 
    play_req_.only_i_frame = false;
    play_req_.speed = 1.0;
    play_req_.range.rangeType = rangeTypeNpt;
    play_req_.range.rangeInfo.npt.start = 0;
    play_req_.range.rangeInfo.npt.end = -1;

    record_req_.range.rangeType = rangeTypeNpt;
    record_req_.range.rangeInfo.npt.start = 0;
    record_req_.range.rangeInfo.npt.end = -1;
    
    common_.user_agent = "";
    common_.url = "";
    common_.session_id = "";
    
    options_req_.seq = 0;
    announce_req_.seq = 0;
    describe_req_.seq = 0;
    play_req_.seq = 0;
    record_req_.seq = 0;
    pause_req_.seq = 0;
    teardown_req_.seq = 0;    
    cur_method_ = rtspMethodMax;        ///> 无效的方法
    setup_req_times_ = 0;
}
