/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Defs.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:32:58
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <stdio.h>
#include "jsoncpp/include/json.h"

#define GenCode(code) ({char buf[9]; snprintf(buf, sizeof(buf), "%08d", code); buf;})

#define MESSAGE_TYPE_CMD    0
#define MESSAGE_TYPE_MEDIA  1
#define MESSAGE_TYPE_RPC    2

#define MAGIC_PRIV    0x40404040  //@@@@

#pragma pack (4)
typedef struct {
    uint32_t    magic;         //@@@@ MAGIC_PRIV
    uint32_t    body_len;
    uint8_t     version;
    uint8_t     flag;
    uint8_t     type;          ///0-信令，1-媒体数据, 2-rpc
    uint8_t     encrypt;       ///加密类型
    uint32_t    sequence;
    uint32_t    session_id;
} PrivateDataHead;

typedef struct {
    uint32_t    magic;
    uint32_t    body_len;
}CommonHeader;

struct Message {
    bool            isResponse;       ///true表示应答，false是请求
    bool            isMediaData;
    uint32_t        sessionId;
    uint32_t        sequence;
    uint32_t        code;             ///只有应答才有此字段
    std::string     message;
    std::string     method;           ///请求才有method
    std::string     event_name;
    Json::Value     data;
    char*           mediaData;
    uint32_t        mediaDataLen;

    Message():isResponse(false),isMediaData(false),
                sessionId(0), sequence(0), data(Json::nullValue), mediaData(nullptr), mediaDataLen(0){
    }
};

#pragma pack ()
