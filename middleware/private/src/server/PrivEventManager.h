/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivEventManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:32:45
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

class PrivEventManager {

private:
    PrivEventManager();
    ~PrivEventManager();

public:
    static PrivEventManager* instance();

    /**
     * @brief 增加可订阅事件
     * @param[in] name  事件名
     */
    bool addSubscribeEvent(const char* name);    

    /**
     * @brief 删除可订阅事件
     * @param[in] name  事件名
     */
    bool delSubscribeEvent(const char* name);  

    /**
     * @brief 是否可订阅事件
     * @param[in] name  事件名
     */
    bool canSubscribeEvent(const char* name);  

private:
    std::mutex             mEventsMutex;
    std::vector<std::string>  mEvents;
};
