/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspMessageParser.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-01 00:01:55
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <cstring>
#include <string>
#include <string.h>
#include "infra/include/Timestamp.h"
#include "RtspMessageParser.h"
#include "infra/include/Logger.h"

static const char* sRtspMethod[rtspMethodMax] = 
{ "OPTIONS", "DESCRIBE", "ANNOUNCE", "SETUP", "PLAY", "RECORD", "PAUSE", "TEARDOWN", "SET_PARAMETER", "GET_PARAMETER"};

static const ResponseText sResponseCodeStr[] = {
    { 200, "OK" },                      /// optimize ok
    { 100, "Continue"},
    { 201, "Created"},
    { 250, "Low on Storage Space"},
    { 300, "Multiple Choices"},
    { 301, "Moved Permanently"},
    { 302, "Moved Temporarily"},
    { 303, "See Other"},
    { 304, "Not Modified"},
    { 305, "Use Proxy"},

    { 400, "Bad Request"},
    { 401, "Unauthorized"},
    { 402, "Payment Required"},
    { 403, "Forbidden"},
    { 404, "Not Found"},
    { 405, "Method Not Allowed"},
    { 406, "Not Acceptable"},
    { 407, "Proxy Authentication Required"},
    { 408, "Request Time-out"},

    { 410, "Gone"},
    { 411, "Length Required"},
    { 412, "Precondition Failed"},
    { 413, "Request Entity Too Large"},
    { 414, "Request-URI Too Large"},
    { 415, "Unsupported Media Type"},

    { 451, "Parameter Not Understood"},
    { 452, "Conference Not Found"},
    { 453, "Not Enough Bandwidth"},
    { 454, "Session Not Found"},
    { 455, "Method Not Valid in This State"},
    { 456, "Header Field Not Valid for Resource"},
    { 457, "Invalid Range"},
    { 458, "Parameter Is Read-Only"},
    { 459, "Aggregate operation not allowed"},
    { 460, "Only aggregate operation allowed"},
    { 461, "Unsupported transport"},
    { 462, "Destination unreachable"},

    { 500, "Internal Server Error"},
    { 501, "Not Implemented"},
    { 502, "Bad Gateway"},
    { 503, "Service Unavailable"},
    { 504, "Gateway Time-out"},
    { 505, "RTSP Version not supported"},
    { 551, "Option not supported"},

    { 0,   nullptr}
};

RtspMessageParser::RtspMessageParser() {
}

RtspMessageParser::~RtspMessageParser() {
    tracef("~RtspMessageParser()\n");
}

int32_t RtspMessageParser::parseRequest(const char* request, int32_t len, RtspMessage &message, int32_t &used) {
    used = len;
    char* cmd_end = std::strstr((char*)request, "\r\n\r\n" );
    if (cmd_end == nullptr) {
        return -1;
    }

    int32_t method_len = int32_t(cmd_end - request + 4);
    int32_t content_len = 0;
    (void)parseSentence(request, "Content-Length:", content_len);
    method_len += content_len;
    used = method_len;

    if (!getUrl(request, message.common_.url)) {
        return -1;
    }

    if (!parseSentence(request, "CSeq:", message.common_.seq)) {
        return -1;
    }

    if (!parseSentence(request, "User-Agent:", message.common_.user_agent)) {
        infof("not found User-Agent\n");
    }

    for (auto i = 0; i < rtspMethodMax; i ++) {
        if (std::strstr((char*)request, sRtspMethod[i])) {
            message.method_ = RtspMethod(i);
            if (i >= rtspMethodMax) {
                message.method_ = rtspMethodMax;
                return -1;
            }
            break;
        }
    }

    switch (message.method_) {
        case rtspMethodOptions:
            parseRequest_Options(request, len, message);
            break;
        case rtspMethodDescribe:
            parseRequest_Describe(request, len, message);
            break;
        case rtspMethodAnnounce:
            parseRequest_Announce(request, len, message);
            break;
        case rtspMethodSetup:
            parseRequest_Setup(request, len, message);
            break;
        case rtspMethodPlay:
            parseRequest_Play(request, len, message);
            break;
        case rtspMethodRecord:
            parseRequest_Record(request, len, message);
            break;
        case rtspMethodPause:
            parseRequest_Pause(request, len, message);
            break;
        case rtspMethodTeardown:
            parseRequest_Teardown(request, len, message);
            break;    
        case rtspMethodSetParameter:
        case rtspMethodGetParameter:
            parseRequest_Parameter(request, len, message);
            break;                                                                                             
    }

    return 0;
}

std::shared_ptr<char> RtspMessageParser::getReply(uint32_t status_code, RtspMessage &message, const std::string &content) {
    RtspMethod method = message.method_;
    switch (method) {
        case rtspMethodOptions:
            return makeResponse_Options(status_code, message);
        case rtspMethodDescribe:
            return makeResponse_Describe(status_code, message, content);
        case rtspMethodAnnounce:
            return makeResponse_Announce(status_code, message);
        case rtspMethodSetup:
            return makeResponse_Setup(status_code, message);
        case rtspMethodPlay:
            return makeResponse_Play(status_code, message);
        case rtspMethodRecord:
            return makeResponse_Record(status_code, message);
        case rtspMethodPause:
            return makeResponse_Pause(status_code, message);
        case rtspMethodTeardown:
            return makeResponse_Teardown(status_code, message);
        case rtspMethodSetParameter:
        case rtspMethodGetParameter:
            return makeResponse_Parameter(status_code, message);                                              
    }
    return nullptr;
}

bool RtspMessageParser::getUrl(const char* request, std::string &url) {
    char buffer[256] = {0};
    char* pos = std::strstr((char*)request, "rtsp://");
    if (pos && std::sscanf(pos, "%s", buffer) == 1) {
        url = buffer;
        return true;
    }
    errorf("rtsp url error, not url\n");
    return true;
}

bool RtspMessageParser::parseSentence(const char *request, const char *key, uint32_t &value) {
    char pattern[256] = {0};
    snprintf(pattern, sizeof(pattern), "%s %%u", key);
    char* pos = std::strstr((char*)request, key);
    return pos && std::sscanf(pos, pattern, &value) == 1;
}

bool RtspMessageParser::parseSentence(const char *request, const char *key, std::string &value) {
    char pattern[256] = {0};
    char buffer[256] = {0};
    snprintf(pattern, sizeof(pattern), "%s %%s\r\n", key);
    char* pos = std::strstr((char*)request, key);
    if (pos && std::sscanf(pos, pattern, buffer) == 1) {
        value = buffer;
        return true;
    }
    return false;
}

bool RtspMessageParser::parseSentence(const char *request, const char *key, int32_t &value) {
    return parseSentence(request, key, (uint32_t&)(value));
}

int32_t RtspMessageParser::parseRequest_Options(const char* request, int32_t len, RtspMessage &message) {
    std::string authInfo;
    if (!parseSentence(request, "Authorization:", authInfo)) {
        return -1;
    }
    tracef("Authorization: %s\n", authInfo.c_str());
    RtspMessage::HeadFieldElement element;
    element.key = "Authorization";
    element.value = authInfo;
    return 0;
}

int32_t RtspMessageParser::parseRequest_Describe(const char* request, int32_t len, RtspMessage &message) {
    return -1;
}

int32_t RtspMessageParser::parseRequest_Setup(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) < 0) {
        return -1;
    }
    std::string transport;
    if (!parseSentence(request, "Transport:", transport)) {
        return -1;
    }

    RtspMessage::HeadFieldElement element = {"Transport", transport};
    message.setup_req_.headField.push_back(element);
    message.setup_req_.Transport = transport;

    if (strstr(transport.c_str(), "RTP/AVP/TCP")) {
        message.setup_req_.transport.proto = rtpProtocolRtpOverRtsp;
        const char *p = strstr(transport.c_str(), "interleaved=");
        if (p) {
            int32_t ret = sscanf(p, "interleaved=%d-%d", &message.setup_req_.transport.cli_rtp_channel, &message.setup_req_.transport.cli_rtcp_channel);
            if (ret != 2) {
                errorf("get tcp interleaved error\n");
                return -1;
            }
        } else {
            errorf("not found interleaved=\n");
            return -1;
        }
    } else if (strstr(transport.c_str(), "RTP/AVP")) {
        message.setup_req_.transport.proto = rtpProtocolRtpOverUdp;
        const char *p = strstr(transport.c_str(), "client_port=");
        if (p) {
            int32_t ret = sscanf(p, "client_port=%d-%d", &message.setup_req_.transport.cli_rtp_channel, &message.setup_req_.transport.cli_rtcp_channel);
            if (ret != 2) {
                errorf("get udp port error\n");
                return -1;
            }
        } else {
            errorf("not found client_port=\n");
            return -1;
        }
    } else {
        message.setup_req_.transport.proto = rtpProtocolNum;
        errorf("unknown transport protocol\n");
    }
    message.setup_req_.transport.multicast = strstr(transport.c_str(), "unicast") == nullptr;
    return 0;
}

int32_t RtspMessageParser::parseRequest_Play(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("session checkout error\n");
    return -1;
}

int32_t RtspMessageParser::parseRequest_Pause(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("session checkout error\n");
    return -1;
}

int32_t RtspMessageParser::parseRequest_Teardown(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("Session checkout error\n");
    return -1;
}

int32_t RtspMessageParser::parseRequest_Parameter(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("session checkout error\n");
    return -1;
}

int32_t RtspMessageParser::parseRequest_Announce(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("session checkout error\n");
    return -1;
}

int32_t RtspMessageParser::parseRequest_Record(const char* request, int32_t len, RtspMessage &message) {
    if (checkSessionId(request, len, message) >= 0) {
        return 0;
    }
    errorf("Session checkou error\n");
    return -1;
}

int32_t RtspMessageParser::checkSessionId(const char* request, int32_t len, RtspMessage &message) {
    std::string sessionId;
    (void)parseSentence(request, "Session:", sessionId);
    if (message.common_.session_id.length() == 0) {
        return 0;
    }
    if (message.common_.session_id != sessionId) {
        errorf("session error\n");
        return -1;
    }
    return 0;
}

void RtspMessageParser::makeSessionId(RtspMessage &message) {
    if (message.common_.session_id.length() != 0){
        return;
    }
    char id[32] = {0};
    snprintf(id, sizeof(id), "%ld", infra::getCurrentTimeMs());
    //message.common_.time_out = 60;
    message.common_.session_id = id;
}

const char* RtspMessageParser::getResponseText(uint32_t code) {
    for (auto it : sResponseCodeStr) {
        if (code == it.code) {
            return it.textInfo;
        }
    }
    return nullptr;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Options(uint32_t status_code, RtspMessage &message) {
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Server: %s\r\n", "Ulucu RtspServer 2.0");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Public:");
    for (auto it : sRtspMethod) {
        cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, " %s,", it);
    }

    cmd_len -= 1;   ///去掉最后一个逗号
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n\r\n");

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr) {
        errorf("res.get() is nullptr\n");
        return nullptr;
    }
    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Describe(uint32_t status_code, RtspMessage &message,
                                                                const std::string &content) {
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Cache-Control: must-revalidate\r\n");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Content-Length: %u\r\n", (int32_t)(content.size()));
    std::string base = message.common_.url;
    if (base[base.size() - 1] != '/') {
        base += "/";
    }

    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Content-Base: %s\r\n", base.c_str());
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Content-Type: application/sdp\r\n");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, content.c_str());

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr) {
        errorf("res.get() is nullptr\n");
        return nullptr;
    }

    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Setup(uint32_t status_code, RtspMessage &message) {
    makeSessionId(message);
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Session: %s;timeout=%d\r\n", message.common_.session_id.c_str(), message.common_.time_out);

    if (message.setup_req_.transport.proto == rtpProtocolRtpOverRtsp) {
        cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d;ssrc=%u\r\n", 
            message.setup_req_.transport.cli_rtp_channel, message.setup_req_.transport.cli_rtcp_channel,
            message.setup_req_.transport.ssrc);
    } else if (message.setup_req_.transport.proto == rtpProtocolRtpOverUdp) {
        cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%u\r\n", 
            message.setup_req_.transport.cli_rtp_channel, message.setup_req_.transport.cli_rtcp_channel,
            message.setup_req_.transport.svr_rtp_channel, message.setup_req_.transport.svr_rtcp_channel,
            message.setup_req_.transport.ssrc);
    }

    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n");

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr) {
        errorf("res.get() is nullptr\n");
        return nullptr;
    }
    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Play(uint32_t status_code, RtspMessage &message) {
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Session: %s\r\n", message.common_.session_id.c_str());
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Range: npt=0.000000-\r\n");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTP-Info: ");

    for (auto it : message.play_req_.info_list) {
        cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "url=trackID=%d;seq=%u;rtptime=%u,", it.mediaIndex, it.seq, it.ts);
    }

    cmd_len -= 1;   ///去掉最后一个逗号
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n");
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n");

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr){
        errorf("res.get() is nullptr\n");
        return nullptr;
    }
    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Pause(uint32_t status_code, RtspMessage &message) {
    return nullptr;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Teardown(uint32_t status_code, RtspMessage &message) {
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Session: %s\r\n", message.common_.session_id.c_str());   
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n"); 

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr){
        errorf("res.get() is nullptr\n");
        return nullptr;
    }
    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Parameter(uint32_t status_code, RtspMessage &message) {
    char buf[1024] = {0};
    int32_t cmd_len = 0;
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "RTSP/1.0 %d %s\r\n", status_code, getResponseText(status_code));
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "CSeq: %d\r\n", message.common_.seq);
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "Session: %s\r\n", message.common_.session_id.c_str());   
    cmd_len += snprintf(buf + cmd_len, sizeof(buf) - cmd_len, "\r\n"); 

    std::shared_ptr<char> res(new char[cmd_len + 1]);
    if (res == nullptr) {
        errorf("res.get() is nullptr\n");
        return nullptr;
    }
    memcpy(res.get(), buf, cmd_len + 1);
    return res;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Announce(uint32_t status_code, RtspMessage &message) {
    return nullptr;
}

std::shared_ptr<char> RtspMessageParser::makeResponse_Record(uint32_t status_code, RtspMessage &message) {
    return nullptr;
}
