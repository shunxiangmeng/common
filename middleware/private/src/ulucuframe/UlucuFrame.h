/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  UlucuFrame.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 16:15:27
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stdint.h>

class UlucuFrame {
public:
    /**
     * @brief 构造
     * @param buffer
     * @param size
     */
    UlucuFrame(uint8_t *buffer, uint32_t size);
    /**
     * @brief 析构
     */
    ~UlucuFrame();
    /**
     * @brief 设置帧序号
     * @param[in] seq 帧序号，视频和音频的序号单独计算
     */
    UlucuFrame& setSequence(uint32_t seq);
    /**
     * @brief 设置UTC时间戳
     * @param[in] dts utc时间，单位是ms
     */
    UlucuFrame& setDtsPts(uint64_t dts, uint64_t pts);
    /**
     * @brief 获取时间戳
     * @return
     */
    uint64_t dts();
    /**
     * @brief 获取时间戳
     * @return
     */
    uint64_t pts();
    /**
     * @brief 设置帧类型
     * @param[in] type 帧类型 'V'-视频帧，'A'-音频帧, 'J'-图片帧
     */
    UlucuFrame& setType(uint8_t type);
    /**
     * @brief 获取帧类型
     * @return
     */
    uint8_t getType();
    /**
     * @brief 设置帧子类型
     * @param[in] type 视频子类型 'I'-I帧，'P'-P帧, 'B'-B帧
     */
    UlucuFrame& setSubType(uint8_t type);
    /**
     * @brief 获取帧子类型
     * @return
     */
    uint8_t getSubType();
    /**
     * @brief 设置视频编码类型，只有I帧才调用这个函数，P帧B帧可以不用调用
     * @param[in] info
     */
    UlucuFrame& setVideoInfo(const VFrameInfo &info);
    /**
     * @brief 设置音频编码类型
     * @param[in] info
     * @return
     */
    UlucuFrame& setAudioInfo(const AFrameInfo &info);
    /**
     * @brief 组私有帧
     * [in]buffer 数据起始地址
     * [in]len 数据长度
     */
    uint32_t makeFrame(uint8_t *buffer, uint32_t len, bool avcchvcc = false);

private:
    uint8_t*    mBuffer;
    uint32_t    mBufferSize;
};
