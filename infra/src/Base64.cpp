/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Base64.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 13:16:19
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/Base64.h"

namespace infra {
static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uint8_t pr2six[256] = {

    /** ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

static inline bool isBase64(uint8_t c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64Encode(uint8_t const* src, uint32_t len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char charArray3[3] = {0};
    unsigned char charArray4[4] = {0};

    while (len--) {
        charArray3[i++] = *(src++);
        if (i == 3) {
            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;
            for (i = 0; (i < 4); i++) {
                ret += base64Chars[charArray4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            charArray3[j] = '\0';
        }

        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) {
            ret += base64Chars[charArray4[j]];
        }
        while ((i++ < 3)) {
            ret += '=';
        }
    }
    return ret;
}

std::string base64Decode(std::string const& src) {
    int32_t srcPosition = (int32_t)src.size();
    uint8_t charArray4[4] = {0};
    uint8_t charArray3[3] = {0};
    std::string ret;

    int32_t i = 0;
    int32_t j = 0;
    int32_t srcIndex = 0;
    while (srcPosition-- && (src[srcIndex] != '=') && isBase64(src[srcIndex])) {
        charArray4[i++] = src[srcIndex]; srcIndex++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                charArray4[i] = (uint8_t)base64Chars.find(charArray4[i]);
            }
            charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];
            for (i = 0; i < 3; i++) {
                ret += charArray3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++) {
            charArray4[j] = (uint8_t)base64Chars.find(charArray4[j]);
        }
        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) {
            ret += charArray3[j];
        }
    }
    return ret;
}

int32_t base64EncodeLength(int32_t length) {
    return ((length + 2) / 3 * 4) + 1;
}

int32_t base64DecodeLength(const std::string& src) {
    int32_t nbytesdecoded = 0;
    register int32_t nprbytes = 0;
    const char* ptr = src.c_str();
    register const char* bufin = src.c_str();
    while (pr2six[*(bufin++)] <= 63);
    nprbytes = int32_t((bufin - ptr) - 1);
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;
    return nbytesdecoded + 1;
}
}