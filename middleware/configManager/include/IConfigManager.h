/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  IConfigManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-12 12:30:55
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <vector>
#include "infra/include/Signal.h"
#include "jsoncpp/include/value.h"

class IConfigManager {
public:
    enum ApplyOptions {
        applyDelaySave = 0,
        applyWithoutLog,
        applyForceNotify,
        applyNoNotify,
    };

    enum ApplyResults {
        applySuccess = 0,
        applyFailed = 0x0000006c,
        applyNeedRestart = 0x00000001,
        applyNeedReboot = 0x00000002,
        applyWriteFileError = 0x00000004,
        applyCapsNotSupport = 0x00000008,
        applyValidateFailed = 0x00000020,
        applyNotExist = 0x00000040,
        applyPartSuccessed = 0x00000080,
        applyNeedRestore = 0x00000800,
        applyTempCfg = 0x00001000,
    };

    typedef infra::TSignal<void, const char*, const Json::Value&, ApplyResults&> ConfigSignal;
    typedef ConfigSignal::Proc ConfigProc;

public:
    IConfigManager() = default;
    ~IConfigManager() = default;
    static IConfigManager* instance();
    virtual bool init(const char* path, const char* default_path) = 0;

    virtual bool getConfig(const char* name, Json::Value& table) = 0;

    virtual bool getDefault(const char* name, Json::Value& table) = 0;
    virtual bool setDefault(const char* name, const Json::Value& table) = 0;

    virtual bool attachApply(const char* name, ConfigProc proc) = 0;
    virtual bool detachApply(const char* name, ConfigProc proc, bool wait = false) = 0;

    virtual bool attachVerify(const char* name, ConfigProc proc) = 0;
    virtual bool detachVerify(const char* name, ConfigProc proc, bool wait = false) = 0;

    virtual bool restore(std::vector<std::string>& members, ApplyResults& results) = 0;
    virtual bool setConfig(const char* name, const Json::Value& table, ApplyResults& results, ApplyOptions options = applyDelaySave) = 0;
    virtual bool importConfig(const Json::Value& table) = 0;
    virtual bool exportConfig(Json::Value& table) = 0;
private:

};