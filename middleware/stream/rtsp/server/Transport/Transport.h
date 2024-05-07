/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Transport.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-04 13:42:57
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <functional>
#include "infra/include/Signal.h"
#include "infra/include/network/Socket.h"
#include "infra/include/network//SocketHandler.h"
#include "common/mediaframe/MediaFrame.h"

typedef infra::TSignal<int32_t, int32_t, const char*, int32_t>::Proc DataProc;
typedef infra::TSignal<void, int32_t>::Proc           ExceptionCallBack;

enum TransportOpt {
    transportOptSndBuffer,            ///> 发送缓冲
    transportOptRecvBuffer,           ///> 接收缓冲
    transportOptRemoteAddr,           ///> 设置对端地址，UDP专用
    transportOptMulticastIF,          ///> 设置组播地址，UDP专用
    transportOptTTL,                  ///> 设置time-to-live，UDP专用
    transportOptLowSpeedRecv,         ///> 设置低速接收模式
    transportOptMediaSsrc,            ///> 设置有效媒体的SSRC
    TransportRtpPacketOptimizing,     ///> 设置是否使用rtp优化接收略， 只对tcp有用
    TransportRtpDisorderWindow,       ///> 设置RTP乱序窗口配置,只对UDP有用
    transportOptRealTimeRecv,         ///> 设置实时接收选项，利用handle_input来接收数据
    transportOptNumber,               ///> 未知
};

class Transport: public infra::SocketHandler {
public:

    enum SocketType {
        socketTypeTcp,   ///tcp
        socketTypeUdp,   ///udp
        socketTypeNumber,
    };

    /**
     * @brief 创建传输对象
     * param [in] sock 
     * param [in] needClose  套接字是否需要内部关闭
     */
    static std::shared_ptr<Transport> create(SocketType sockType, std::shared_ptr<infra::Socket> &sock, bool needClose);
    /**
     * @brief 设置tcp/udp共有的传输选项
     * param [in] optname-选项名
     * param [in] optvalue-选项值
     * param [in] len-选项长度
     * return -1-失败，0-成功
     */
    virtual int32_t setOption(TransportOpt optName, void *optValue, int32_t len) { return -1; }
    /**
     * @brief 销毁
     */
    virtual void destroy() {}
    /**
     * @brief 启动接收数据
     */
    virtual bool startReceive() = 0;
    /**
     * @brief 停止接收数据
     */
    virtual bool stopReceive() = 0;
    /**
     * @brief 不设置物理通道对应的通道号
     * 仅在udp方式下需要设置，tcp设置无效
     */
    virtual int32_t setChannelId(int channel_id) { return -1; }
    /**
     * @brief 发送数据
     * @param data
     * @param dataLen
     * @return
     */
    virtual int32_t send(const char *data, int32_t size) = 0;
    /**
     * @brief 设置数据callback
     * @param dataCallback
     * @return
     */
    int32_t setDataCallback(DataProc dataCallback);

    /**
     * @brief 设置异常回调
     * @param callback
     * @return
     */
    int32_t setExceptionCallback(ExceptionCallBack callback);

protected:
    /**
     * @brief 构造函数
     * 不允许外部直接创建CTransport
     */
    Transport();
    /**
     * @brief 析构
     */
    virtual ~Transport();

protected:
    bool                    mNeedCloseSock;
    int32_t                 mChannelId;
    int32_t                 mPort;
    DataProc                mDataCallback;
    ExceptionCallBack       exception_callback_;
};
