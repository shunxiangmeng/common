/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ConfigManager.h
 * Author      :  mengshunxiang 
 * Data        :  2024-05-12 13:04:38
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <string>
#include <map>
#include <memory>
#include "configManager/include/IConfigManager.h"
#include "jsoncpp/include/value.h"
#include "infra/include/Timer.h"

class ConfigManager : public IConfigManager {
    friend class IConfigManager;
    ConfigManager();
    virtual ~ConfigManager();

public:
    virtual bool init(const char* path, const char* default_path) override;

    virtual bool getConfig(const char* name, Json::Value& table) override;

    virtual bool getDefault(const char* name, Json::Value& table) override;
    virtual bool setDefault(const char* name, const Json::Value& table) override;

    virtual bool attachApply(const char* name, ConfigProc proc) override;
    virtual bool detachApply(const char* name, ConfigProc proc, bool wait = false) override;

    virtual bool attachVerify(const char* name, ConfigProc proc) override;
    virtual bool detachVerify(const char* name, ConfigProc proc, bool wait = false) override;

    virtual bool restore(std::vector<std::string>& members, ApplyResults& results) override;
    virtual bool setConfig(const char* name, const Json::Value& table, ApplyResults& results, ApplyOptions options = applyDelaySave) override;
    virtual bool importConfig(const Json::Value& table) override;
    virtual bool exportConfig(Json::Value& table) override;


private:
    std::string getFirstGradeConfigName(const char* name);
    bool readFile(const std::string& path, Json::Value &config);
    bool saveFile(void);
    bool onApplyProc(const char* name, const Json::Value &table, ApplyResults &results);
    bool onVerifyProc(const char* name, const Json::Value &table, ApplyResults &results);
private:
    bool changed_;

    Json::Value config_;            ///< Json配置总表
    Json::Value config_default_;    ///< Json默认配置表

    std::string config_file_path_;          ///< 配置文件路径
    std::string config_default_file_path_;  ///< 默认配置文件路径

    infra::Timer timer_;

    typedef std::map<std::string, std::shared_ptr<ConfigSignal>> ConfigSignalMap;
    ConfigSignalMap config_verify_map_;
    ConfigSignalMap config_apply_map_;
    std::mutex mutex_;
};

