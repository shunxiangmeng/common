/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  RtspMessageParser.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-03 21:56:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <memory>
#include "RtspMessage.h"

class RtspMessageParser {
public:
    RtspMessageParser();
    ~RtspMessageParser();

    int32_t parseRequest(const char* request, int32_t len, RtspMessage &message, int32_t &used);

    std::shared_ptr<char> getReply(uint32_t status_code, RtspMessage &message, const std::string &content = "");

    static const char* getMethodString(RtspMethod method);

private:

    const char* getResponseText(uint32_t code);

    bool parseSentence(const char *request, const char *key, int32_t &value);
    bool parseSentence(const char *request, const char *key, uint32_t &value);
    bool parseSentence(const char *request, const char *key, std::string &value);
    bool getUrl(const char* request, std::string &url);
    int32_t checkSessionId(const char* request, int32_t len, RtspMessage &message);
    void makeSessionId(RtspMessage &message);

    int32_t parseRequest_Authorization(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Options(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Describe(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Setup(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Play(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Pause(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Teardown(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Parameter(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Announce(const char* request, int32_t len, RtspMessage &message);
    int32_t parseRequest_Record(const char* request, int32_t len, RtspMessage &message);

    std::shared_ptr<char> makeResponse_Authorization(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Options(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Describe(uint32_t status_code, RtspMessage &message, const std::string &content);
    std::shared_ptr<char> makeResponse_Setup(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Play(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Pause(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Teardown(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Parameter(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Announce(uint32_t status_code, RtspMessage &message);
    std::shared_ptr<char> makeResponse_Record(uint32_t status_code, RtspMessage &message);  
};
