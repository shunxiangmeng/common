/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  ConfigManager.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-12 12:43:05
 * Description :  None
 * Note        : 
 ************************************************************************/
#include <algorithm>
#include "ConfigManager.h"
#include "infra/include/Logger.h"
#include "infra/include/File.h"
#include "infra/include/Utils.h"
#include "jsoncpp/include/reader.h"
#include "jsoncpp/include/writer.h"

#define CONFIG_FILE "config.json"
#define DEFAULT_CONFIG_FILE "default_config.json"

IConfigManager* IConfigManager::instance() {
    static ConfigManager s_config_manager;
    return &s_config_manager;
}

ConfigManager::ConfigManager() : changed_(false) {
}

ConfigManager::~ConfigManager() {
}

bool ConfigManager::init(const char* path, const char* default_path) {
    if (path == nullptr || default_path == nullptr) {
        errorf("path is null or default_path is null\n");
        return false;
    }
    config_file_path_ = path;
    config_default_file_path_ = default_path;

    /*if (config_file_path_ == "") {
        config_file_path_ = infra::exePath();
    }
    if (config_default_file_path_ == "") {
        config_default_file_path_ = infra::exePath();
    }*/

    config_file_path_ += CONFIG_FILE;
    config_default_file_path_ += DEFAULT_CONFIG_FILE;

    bool has_config = readFile(config_file_path_, config_);
    bool has_default_config = readFile(config_default_file_path_, config_default_);

    if (!has_config && !has_default_config) {
        errorf("there is no config file and default_config file\n");
        return false;
    }

    if (!has_config && has_default_config) {
        warnf("use default config\n");
        config_ = config_default_;
        changed_ = true;
        saveFile();
    }

    return true;
}

bool ConfigManager::getConfig(const char *name, Json::Value &table) {
    if (name == nullptr || strlen(name) == 0) {
        warnf("config name is null or empty, getConfig failed.\n");
        return false;
    }

    std::lock_guard<std::mutex> guard(mutex_);
    if (strcmp(name, "all") == 0) {
        table = config_;
        return table.type() != Json::nullValue;
    }
    table = Json::Path(name).resolve(config_);
    if (table.type() == Json::nullValue) {
        warnf("%s is Json::nullValue!\n", name);
    }
    return true;
}

bool ConfigManager::getDefault(const char* name, Json::Value& table) {
    return true;
}

bool ConfigManager::setDefault(const char* name, const Json::Value& table) {
    return true;
}

bool ConfigManager::setConfig(const char* name, const Json::Value& table, ApplyResults& results, ApplyOptions options) {
    if (name == nullptr || strlen(name) == 0) {
        errorf("setDefault error with name is null or empty.\n");
        return false;
    }
    std::string config_name = getFirstGradeConfigName(name);

    Json::Path config_path(name);
    //Json::Value cur_config = config_[config_path];
    Json::Value& config_ref = config_path.make(config_);
    //tracef("old_config:%s\n", config_ref.toStyledString().data());
    if (config_ref == table) {
        return true;
    }

    Json::Value old_table = config_ref;

    ApplyResults apply_results = applySuccess;
    if (!onVerifyProc(name, table, apply_results)) {
        errorf("Config: [%s] VerifyProc failed, result is: [%d].\n", config_name.c_str(), apply_results);
        return false;
    }
    if (!onApplyProc(name, table, apply_results)) {
        errorf("Config: [%s] ApplyProc failed, result is: [%d].\n", config_name.c_str(), apply_results);
        return false;
    }
    if (apply_results == applySuccess) {
        config_[config_name] = table;
        changed_ = true;
        saveFile();
    }
    return true;
}

bool ConfigManager::attachApply(const char* name, ConfigProc proc) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_apply_map_.find(name);
    if (it == config_apply_map_.end()) {
        auto signal_ptr = std::make_shared<ConfigSignal>(64);
        it = config_apply_map_.insert(ConfigSignalMap::value_type(name, signal_ptr)).first;
    } 
    int ret = it->second->attach(proc);
    if (ret < 0) {
        errorf("signal attach ret:%d\n", ret);
    }
    return ret > 0;
}

bool ConfigManager::detachApply(const char* name, ConfigProc proc, bool wait) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_apply_map_.find(name);
    if (it == config_apply_map_.end()) {
        return false;
    }
    return it->second->detach(proc, wait) > 0;
}

bool ConfigManager::attachVerify(const char* name, ConfigProc proc) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_verify_map_.find(name);
    if (it == config_verify_map_.end()) {
        auto signal_ptr = std::make_shared<ConfigSignal>(64);
        it = config_verify_map_.insert(ConfigSignalMap::value_type(name, signal_ptr)).first;
    } 
    int ret = it->second->attach(proc);
    if (ret < 0) {
        errorf("signal attach ret:%d\n", ret);
    }
    return ret > 0;
}

bool ConfigManager::detachVerify(const char* name, ConfigProc proc, bool wait) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_verify_map_.find(name);
    if (it == config_verify_map_.end()) {
        return false;
    }
    return it->second->detach(proc, wait) > 0;
}

std::string ConfigManager::getFirstGradeConfigName(const char* name) {
    std::string find_str = ".[]";
    std::string ret = name;
    std::string::iterator iter = std::find_first_of(ret.begin(), ret.end(), find_str.begin(), find_str.end());
    if (iter != ret.end()) {
        ret = std::string(ret.begin(), iter);
    }
    return ret;
}

bool ConfigManager::onVerifyProc(const char* name, const Json::Value &table, ApplyResults &results) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_verify_map_.find(name);
    if (it == config_verify_map_.end()) {
        return true;
    }
    (*it->second)(name, table, results);
    return results == applySuccess ? true : false;
}

bool ConfigManager::onApplyProc(const char* name, const Json::Value &table, ApplyResults &results) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = config_apply_map_.find(name);
    if (it == config_apply_map_.end()) {
        return true;
    }
    (*it->second)(name, table, results);
    return results == applySuccess ? true : false;
}

bool ConfigManager::restore(std::vector<std::string>& members, ApplyResults& results) {
    return true;
}

bool ConfigManager::importConfig(const Json::Value& table) {
    return true;
}

bool ConfigManager::exportConfig(Json::Value& table) {
    return true;
}

bool ConfigManager::readFile(const std::string& path, Json::Value &config) {
    if (path.empty()) {
        errorf("path empty\n");
        return false;
    }

    Json::Reader reader;
    std::string stream = infra::File::loadFile(path);
    if (stream.length() == 0) {
        errorf("load file(%s) fail\n", path.c_str());
        return false;
    }
    return reader.parse(stream, config);
}

bool ConfigManager::saveFile(void) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (changed_) {
        changed_ = false;
        Json::FastWriter writer;
        std::string stream = writer.write(config_);
        if (stream.empty()) {
            warnf("Json::FastWriter writer stream empty\n");
        } else {
            std::shared_ptr<FILE> fp;
            fp.reset(fopen(config_file_path_.data(), "wb"), [](FILE *fp) {
                if (fp) fclose(fp);
            });
            if (!fp) {
                errorf("create file %s failed\n", config_file_path_.data());
                return false;
            }
            size_t ret = fwrite(stream.data(), sizeof(char), stream.length(), fp.get());
            if (ret < stream.length()) {
                errorf("write config file length %d < content length %d\n", ret, stream.length());
                return false;
            }
            ::fflush(fp.get());
        }
    }
    return true;
}