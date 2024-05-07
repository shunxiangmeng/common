/************************************************
 * Copyright(c) 2021 uni-ubi
 * 
 * Project:    Rtsp
 * FileName:   TransportChannelInterleave.h
 * Author:     jinlun
 * Email:      jinlun@uni-ubi.com
 * Version:    V1.0.0
 * Date:       2021-02-03
 * Description: 
 * Others:
 *************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <memory>
#include <mutex>
#include "infra/include/Signal.h"
#include "TransportChannel.h"

class TransportChannelInterleave : public TransportChannel{
public:
    TransportChannelInterleave();
    ~TransportChannelInterleave();

    /**
     * @brief 设置传输套接字
     * param [in] sock-套接字对象
     * param [in] needClose-套接字是否关闭
     * return >0-设置成功, -1-设置失败
     */
    int32_t setChannelSock(std::shared_ptr<infra::Socket> sock, bool needClose = true);

    /**
     * @brief 设置数据回调
     * param [in] callback: 媒体数据回调接口
     * return -1: 设置失败，0: 设置成功
     */
    virtual int32_t setDataCallback(DataProc callback);

    /**
     * @brief 设置异常回调
     * param [in] callback: 异常回调接口
     * return -1: 设置失败，0: 设置成功
     */
    virtual int32_t setExceptionCallback(ExceptionCallBack callback);

    /**
     * @brief 启动接收数据
     * return false: 失败，true: 成功
     */
    virtual bool start();

    /**
     * @brief 停止接收数据
     * return false: 失败，true: 成功
     */
    virtual bool stop();

    /**
     * @brief 发送数据
     * param [in] frame-数据承载体
     * param [in] channel_id-传输通道标识
     * param [in] mark-标志位，若当前包是帧的最后一个包，则mark == 1,否则mark == 0
     * return >0-已发送的字节数, -1-failed
     */
    virtual int32_t sendMedia(int32_t channelId, const MediaFrame &frame, int32_t mark = 1);

    /**
     * @brief 发送命令数据
     * param [in] data-命令数据
     * param [in] len-命令数据长度
     * return >0-已发送的字节数, -1-failed
     */
    virtual int32_t sendCommand(const char* data, int32_t len);

     /**
     * @brief 获取已发送的统计信息
     * param [in] channel_id-传输通道标识，tcp通道无意义
     * param [out] sendPackets-已发送的总包数
     * param [out] sendBytes-已发送的总字节数
     * return true-成功, false-失败
     */
    virtual void getSendStatistic(int32_t channelId, uint32_t &sendPackets, uint32_t &sendBytes);

protected:

    std::shared_ptr<Transport>  mTransport;  ///tcp
    uint32_t       mSentPackets;
    uint32_t       mSentBytes;
};
