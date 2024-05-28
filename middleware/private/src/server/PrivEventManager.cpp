/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivEventManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:32:53
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivEventManager.h"
#include "infra/include/Logger.h"

PrivEventManager* PrivEventManager::instance() {
    static PrivEventManager manager;
    return &manager;
}

PrivEventManager::PrivEventManager(){
    std::lock_guard<std::mutex> guard(mEventsMutex);
    mEvents.clear();
}

PrivEventManager::~PrivEventManager(){
    std::lock_guard<std::mutex> guard(mEventsMutex);
    mEvents.clear();
}

bool PrivEventManager::addSubscribeEvent(const char* name) {
    if (name == nullptr) {
        errorf("addSubscribeEvent failed, name is nullpre\n");
    }
    std::lock_guard<std::mutex> guard(mEventsMutex);
    std::vector<std::string>::iterator it = std::find(mEvents.begin(), mEvents.end(), name);
    if (it == mEvents.end()) {
        mEvents.push_back(name);
        return true;
    } else {
        warnf("event %S had insert to mEvents\n", name);
        return false;
    }
}

bool PrivEventManager::delSubscribeEvent(const char* name) {
    if (name == nullptr) {
        errorf("delSubscribeEvent failed, name is nullpre\n");
    }
    std::lock_guard<std::mutex> guard(mEventsMutex);
    std::vector<std::string>::iterator it = std::find(mEvents.begin(), mEvents.end(), name);
    if (it != mEvents.end()) {
        mEvents.erase(it);
        return true;
    } else {
        warnf("event %S had not inserted to mEvents\n", name);
        return false;
    }
}

bool PrivEventManager::canSubscribeEvent(const char* name) {
    if (name == nullptr) {
        errorf("canSubscribeEvent failed, name is nullpre\n");
    }
    std::lock_guard<std::mutex> guard(mEventsMutex);
    std::vector<std::string>::iterator it = std::find(mEvents.begin(), mEvents.end(), name);
    return it != mEvents.end();
}
