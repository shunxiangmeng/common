/************************************************
 * Copyright(c) 2021 uni-ubi
 * 
 * Project:    Rtsp
 * FileName:   TransportChannelIndepent.h
 * Author:     jinlun
 * Email:      jinlun@uni-ubi.com
 * Version:    V1.0.0
 * Date:       2021-02-07
 * Description: 
 * Others:
 *************************************************/
#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include "infra/include/Signal.h"
#include "TransportChannel.h"
#include "rtsp/common/Defines.h"

class TransportChannelIndepent: public TransportChannel {
public:
    TransportChannelIndepent();
    virtual ~TransportChannelIndepent();

    static TransportChannelIndepent* create();

    /**
     * @brief 设置传输套接字
     * param [in] sock-套接字对象
     * param [in] needClose-套接字是否关闭
     * return >0-设置成功, -1-设置失败
     */
    int32_t setChannelSock(std::shared_ptr<infra::Socket> sock, bool need_close = true);

    /**
     * @brief 设置媒体数据传输通道
     * param [in] sock-套接字对象
     * param [in] channelId-trackID
     * param [in] remoteIp-远端IP
     * param [in] remotePort-远端端口
     * param [in] needClose-套接字是否关闭
     * return >0-设置成功, -1-设置失败
     */
    int32_t addDataChannel(int32_t channelId, std::shared_ptr<infra::Socket> rtpsock, std::shared_ptr<infra::Socket> rtcpsock, 
        const char* remoteIp, int32_t remotertpPort, int32_t remotertcpPort, bool needClose = true);
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
     * @brief 发送RTCP包，只有udp才实现这个接口
     * param [in] frame-数据承载体
     * param [in] channel_id-传输通道标识
     * return >0-已发送的字节数, -1-failed
     */
    virtual int32_t sendRtcp(int32_t channelId, const MediaFrame &frame);
     /**
     * @brief 获取已发送的统计信息
     * param [in] channel_id-传输通道标识，tcp通道无意义
     * param [out] sendPackets-已发送的总包数
     * param [out] sendBytes-已发送的总字节数
     * return true-成功, false-失败
     */
    virtual void getSendStatistic(int32_t channelId, uint32_t &sendPackets, uint32_t &sendBytes);
protected:
    typedef struct {
        std::shared_ptr<Transport> rtpTransport;
        std::shared_ptr<Transport> rtcpTransport;
    } IndepentTransport;

    IndepentTransport mTransports[trackMax];
    uint32_t        mSentPackets[trackMax];
    uint32_t        mSentBytes[trackMax];
    int32_t  channel_id_;
};
