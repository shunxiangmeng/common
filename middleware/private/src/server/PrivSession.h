/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivSession.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:32:27
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include "../Defs.h"
#include "jsoncpp/include/json.h"
#include "PrivSubSession.h"
#include "PrivSessionBase.h"
#include "stream/mediasession/MediaSession.h"
#include "infra/include/Timer.h"

class PrivSession;
class IPrivSessionManager {
public:
    /**
     * @brief 析构
     */
    virtual ~IPrivSessionManager() {}
    /**
     * @brief 回收session
     * @param session
     * @return
     */
    virtual int32_t remove(uint32_t session_id) = 0;
};

class PrivSession : public PrivSessionBase/*, public std::enable_shared_from_this<PrivSession>*/ {
public:
    /**
     * @brief 构造
     * @param manager
     * @param name
     * @param sessionId
     */
    PrivSession(IPrivSessionManager *manager, const char* name, uint32_t session_id);
    /**
     * @brief 析构
     */
    virtual ~PrivSession();
    /**
     * @brief 关闭会话
     */
    void closeSession();
    /**
     * @brief 消息进入
     * @param message
     */
    virtual void onRequest(MessagePtr &message);
    /**
     * @brief 数据接收
     * @param msg
     * @param buffer
     * @param size
     */
    virtual void onMediaData(MessagePtr &msg, const char *buffer, uint32_t size);
    /**
     * @brief 获取视频能力级
     * @param channel
     * @param streamType
     * @param config
     */
    static void getVideoCapability(int32_t channel, int32_t streamType, Json::Value &config);
    /**
     * @brief 查看流状态
     * @return
     */
    bool streamStatus();
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event  json格式的事件
     */
    bool sendEvent(const char* name, const Json::Value &event);
private:
    /**
     * @brief 开始预览
     * @param msg
     * @return
     */
    bool startPreview(MessagePtr &msg);
    /**
     * @brief 停止预览
     * @param msg
     * @return
     */
    bool stopPreview(MessagePtr &msg);
    /**
     * @brief 处理视频能力级别
     * @param msg
     * @return
     */
    bool dealVideoCapability(MessagePtr &msg);
    /**
     * @brief 处理视频配置
     * @param msg
     * @return
     */
    bool dealVideoConfig(MessagePtr &msg);
    /**
     * @brief 开始语音对讲
     * @param msg
     * @return
     */
    bool startTalkBack(MessagePtr &msg);
    /**
     * @brief 停止对讲
     * @param msg
     * @return
     */
    bool stopTalkBack(MessagePtr &msg);
    /**
     * @brief 订阅智能事件
     * @param msg
     * @return
     */
    bool subscribeSmartEvent(MessagePtr &msg);
    /**
     * @brief 推送智能事件
     * @param
     * @return
     */
    bool pushSmartEvent();
    /**
     * @brief 获取通道流信息
     * @param msg
     * @param channel
     * @param streamType
     * @return
     */
    bool getChannelStreamType(MessagePtr &msg, int32_t &channel, int32_t &streamType);
    /**
     * @brief 添加子会话
     * @param[in] channel
     * @param[in] stream
     * @return
     */
    PrivSubSessionPtr addNewSubSession(int32_t channel, int32_t stream);
    /**
     * @brief 获取子会话
     * @param channel
     * @param stream
     * @return
     */
    PrivSubSessionPtr getSubSession(int32_t channel, int32_t stream);

    /**
     * @brief 子会话发送失败
     * @return
     */
    void subSessionSendFailed(std::string error);

private:

protected:

    IPrivSessionManager   *mSessionManager;

private:

    typedef std::pair<int32_t, int32_t>             SessionPair;
    typedef std::map<SessionPair, PrivSubSessionPtr> PrivSessionMap;

    bool                mStreamAlive;            ///正在发送流
    PrivSessionMap      mSubSessionMap;          ///异步销毁
    std::mutex       mSessionMapMtx;          ///子会话map锁

    std::vector<std::string>  mSubscribedEvents;    ///已订阅的事件
    std::mutex             mSubscribedEventsMtx; ///已订阅的事件锁


    //--------------for test
    infra::Timer       mSmartEventTimer;        ///推送事件定时器
    void sendSmartEventProc(uint64_t arg);

};
