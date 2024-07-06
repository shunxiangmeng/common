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
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>
#include "../Defs.h"
#include "jsoncpp/include/json.h"
#include "PrivSubSession.h"
#include "PrivSessionBase.h"
#include "PrivHandler.h"
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
    PrivSession(IPrivSessionManager *manager, const char* name, uint32_t session_id, RPCServer *rpc_server);
    /**
     * @brief 析构
     */
    virtual ~PrivSession();
    /**
     * @brief 关闭会话
     */
    void closeSession();

    virtual bool initial(std::shared_ptr<infra::TcpSocket>& newSock, int32_t bufferLen, bool bRecv = true) override;
    virtual bool initial(std::shared_ptr<infra::TcpSocket>& newSock, const char *buffer, int32_t len) override;
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
    /**
     * @brief 发送事件
     * @param[in] name  事件名
     * @param[in] event string格式的事件
     */
    bool sendEvent(const char* name, const std::string &event);
private:
    void initMethodList();
    bool login(MessagePtr &msg);
    bool start_preview(MessagePtr &msg);
    bool stop_preview(MessagePtr &msg);
    bool dealVideoCapability(MessagePtr &msg);
    bool dealVideoConfig(MessagePtr &msg);
    bool startTalkBack(MessagePtr &msg);
    bool stopTalkBack(MessagePtr &msg);
    bool subscribe_event(MessagePtr &msg);
    bool pushSmartEvent();
    bool getChannelStreamType(MessagePtr &msg, int32_t &channel, int32_t &streamType);

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

    virtual void onDisconnect() override;

private:
    bool call(std::string key, MessagePtr &message);

    template <typename Function, typename Self>
    void registerMethodFunc(std::string&& key, const Function &f, Self *self) {
        this->map_invokers_[key] = [f, self](MessagePtr &message) {
            return (*self.*f)(message);
        };
    }

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

    std::unordered_map<std::string, std::function<bool(MessagePtr &)>> map_invokers_;
    PrivHandler privhandler_;
};
