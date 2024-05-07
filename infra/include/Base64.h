/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Base64.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 13:15:07
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>

namespace infra {

/**
 * @brief base64 编码
 * @param[in]   src 输入源
 * @param[in]   len 输入源长度
 * @return
 */
std::string base64Encode(uint8_t const* src, uint32_t len);

/**
 * @brief base64解码
 * @param[in] src   输入源
 * @return
 */
std::string base64Decode(std::string const& src);

/**
 * @brief 计算编码后数据的缓冲区的长度
 * @param[in] length 输入长度
 * @return
 */
int32_t base64EncodeLength(int32_t length);

/**
 * @brief 计算解码后缓冲区长度
 * @param[in] src   输入源
 * @return
 */
int32_t base64DecodeLength(const std::string& src);

}