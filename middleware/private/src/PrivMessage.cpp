/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivMessage.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-27 19:25:26
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivMessage.h"
#include "jsoncpp/include/json.h"
#include "jsoncpp/include/value.h"
#include "infra/include/Logger.h"

std::shared_ptr<Message> parseBuffer(const char* buffer, int32_t len, int32_t &used_len) {
    used_len = 0;
    if (buffer == nullptr || len < sizeof(PrivateDataHead)) {
        errorf("buffer is nullptr or len(%d) is too small\n", len);
        return nullptr;
    }

    ///查找消息头@@@@
    int32_t i = 0U;
    for (; i < len - 3; i++) {
        if (buffer[i+0] == '@' && buffer[i+1] == '@' && buffer[i+2] == '@' && buffer[i+3] == '@') {
            break;
        }
    }

    if (i >= (len - 3)) {
        errorf("not found head tag '@@@@' \n");
        used_len = len;   ///消耗掉所有数据
        return nullptr;
    }

    if (i > 0) {
        warnf("head tag '@@@@' start with %d\n", i);
        used_len += i;
    }

    char *pBuffer = (char*)&buffer[used_len];
    auto *head = reinterpret_cast<PrivateDataHead*>((char*)pBuffer);
    uint32_t bodyLen = infra::ntohl(head->bodyLen);
    int32_t frameLen = bodyLen + sizeof(PrivateDataHead);

    ///不满一帧数据
    if ((int32_t)len < frameLen) {
        warnf("not enough an frame, frameLen:%d, dalalen:%d\n", frameLen, len);
        return nullptr;
    }

    ///消耗掉一帧数据
    used_len += frameLen;
    std::shared_ptr<Message> message = std::make_shared<Message>();
    message->isResponse = head->flag & 0x40 ? true : false;
    message->sequence  = infra::ntohs(head->sequence);
    message->sessionId = infra::ntohl(head->sessionId);

    ///媒体数据
    if (head->type == 0x01) {
        message->isMediaData = true;
        message->mediaData = head->buf;
        message->mediaDataLen = bodyLen;
        return message;
    } else {
        ///信令数据
        Json::Value root = Json::nullValue;
        Json::String err;
        Json::CharReaderBuilder readbuilder;
        std::unique_ptr<Json::CharReader> jsonreader(readbuilder.newCharReader());
        jsonreader->parse(head->buf, head->buf + bodyLen, &root, &err);
        if (root.empty()) {
            return nullptr;
        }

        infof("recv data content:\n%s", root.toStyledString().c_str());
        message->isResponse = root.isMember("code");
        message->method     = root.isMember("method") ? root["method"].asString() : "";
        message->data       = root.isMember("data") ? root["data"] : Json::nullValue;
    }
    return message;
}