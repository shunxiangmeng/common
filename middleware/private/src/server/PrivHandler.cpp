/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  PrivHandler.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-07-06 12:02:43
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "PrivHandler.h"
#include "hal/Image.h"
#include "hal/Video.h"
#include "hal/Audio.h"
#include "configManager/include/IConfigManager.h"

#define JSON_IS_TYPE(type) is##type()
#define JSON_AS_TYPE(type) as##type()

#define CHECK_PARAMETER(json, x, type, msg)              \
    {                                                    \
        char tmp[128] = {0};                             \
        if (!json.isMember(x)) {                         \
            msg->code = 1001;                            \
            snprintf(tmp, sizeof(tmp), "missing parameter %s", x); \
            msg->message = tmp;                          \
            return false;                                \
        }                                                \
        if (!json[x].JSON_IS_TYPE(type)) {               \
            msg->code = 1002;                            \
            snprintf(tmp, sizeof(tmp), "%s wrong parameter type, need %s", x, #type); \
            msg->message = tmp;                          \
            return false;                                \
        }                                                \
    }

#define GET_AND_CHECK_PARAMETER(__result, json, x, type, msg)  \
    {                                                    \
        char tmp[128] = {0};                             \
        if (!json.isMember(x)) {                    \
            msg->code = 1001;                            \
            snprintf(tmp, sizeof(tmp), "missing parameter %s", x); \
            msg->message = tmp;                          \
            return false;                                \
        }                                                \
        if (!json[x].JSON_IS_TYPE(type)) {          \
            msg->code = 1002;                            \
            snprintf(tmp, sizeof(tmp), "%s wrong parameter type, need %s", x, #type); \
            msg->message = tmp;                          \
            return false;                                \
        }                                                \
        __result = json[x].JSON_AS_TYPE(type);      \
    }

#define IF_GET_PARAMETER(__result, json, x, type, msg)         \
        if (json.isMember(x) && json[x].JSON_IS_TYPE(type)) {   \
            __result = json[x].JSON_AS_TYPE(type);                   \
        }

static std::string toLowercase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

bool PrivHandler::set_video_format(MessagePtr &msg) {
    Json::Value &data = msg->data;
    std::string video_format;
    GET_AND_CHECK_PARAMETER(video_format, data, "video_format", String, msg);
    int fps = 25;
    video_format = toLowercase(video_format);
    if (video_format == "pal") {
        fps = 25;
    } else if (video_format == "ntsc") {
        fps = 30;
    } else {
        msg->code = 1003;
        msg->message = "parameter range error, need [pal, ntsc]";
        return false;
    }
    if (!hal::IImage::instance()->setInputFramerate(0, fps)) {
        msg->code = 1003;
        msg->message = "hal error";
        return false;
    }
    return true;
}

bool PrivHandler::get_video_format(MessagePtr &msg) {
    Json::Value data;
    int fps = hal::IImage::instance()->getInputFramerate(0);
    tracef("image fps:%d\n", fps);
    if (fps == 25) {
        data["video_format"] = "pal";
    } else if (fps == 30) {
        data["video_format"] = "ntsc";
    } else {
        msg->code = 1003;
        msg->message = "hal error";
        return false;
    }
    msg->data = data;
    return true;
}

/**
 * {
 *     "method": "get_video_config",
 *     "data": {
 *         "video_config" : [
 *         {
 *             "channel": 0,
 *             "sub_channel": 0,
 *             "codec": "H264",
 *             "resolution": "1920x1080",
 *             "gop": 50,
 *             "fps": 25.0,
 *             "bitrate_type": "vbr",
 *             "bitrate": 1024,
 *         }
 *     ]}
 * }
 * */
#if 0
bool PrivHandler::get_video_config(MessagePtr &msg) {
    Json::Value data = Json::objectValue;
    Json::Value video_config = Json::arrayValue;
    int32_t stream_count = 0;
    for (int32_t i = 0; i < 5; i++) {
        if (hal::IVideo::instance()->streamIsStarted(0, i)) {
            stream_count++;
            hal::VideoEncodeParams video_params;
            Json::Value config;
            config["channel"] = 0;
            config["sub_channel"] = i;
            if (!hal::IVideo::instance()->getEncodeParams(0, i, video_params)) {
                errorf("get encoded params %d:%d failed\n", 0, i);
                continue;
            }

            if (video_params.codec.has_value()) {
                if (*video_params.codec == H264) {
                    config["codec"] = "H264";
                } else if (*video_params.codec == H265) {
                    config["codec"] = "H265";
                }
            }

            if (video_params.width.has_value() && video_params.height.has_value()) {
                char tmp[32] = {0};
                snprintf(tmp, sizeof(tmp), "%dx%d", *video_params.width, *video_params.height);
                config["resolution"] = tmp;
            }

            if (video_params.gop.has_value()) {
                config["gop"] = *video_params.gop;
            }
            if (video_params.fps.has_value()) {
                config["fps"] = *video_params.fps;
            }
            if (video_params.bitrate.has_value()) {
                config["bitrate"] = *video_params.bitrate;
            }

            if (video_params.rc_mode.has_value()) {
                if (*video_params.rc_mode == VideoCBR) {
                    config["bitrate_type"] = "CBR";
                } else if (*video_params.rc_mode == VideoVBR) {
                    config["bitrate_type"] = "VBR";
                }
            }

            video_config.append(config);
        }
        data["video_config"] = video_config;
    }
    data["stream_count"] = stream_count;
    msg->data = data;
    return true;
}
bool PrivHandler::set_video_config(MessagePtr &msg) {
    CHECK_PARAMETER(msg->data, "video_config", Array, msg);
    Json::Value &config = msg->data["video_config"];
    for (auto &it : config) {
        hal::VideoEncodeParams video_params;
        int32_t channel = 0;
        int32_t sub_channel = 0;
        IF_GET_PARAMETER(channel, it, "channel", Int, msg);
        GET_AND_CHECK_PARAMETER(sub_channel, it, "sub_channel", Int, msg);

        std::string codec;
        IF_GET_PARAMETER(codec, it, "codec", String, msg);
        codec = toLowercase(codec);
        if (codec == "h264") {
            video_params.codec = VideoCodecType::H264;
        } else if (codec == "h265") {
            video_params.codec = VideoCodecType::H265;
        }

        if (it.isMember("resolution") && it["resolution"].isString()) {
            std::string resolution = it["resolution"].asString();
            int32_t width, height;
            if (sscanf(resolution.data(), "%dx%d", &width, &height) == 2) {
                video_params.width = width;
                video_params.height = height;
            }
        }

        if (it.isMember("gop") && it["gop"].isInt()) {
            video_params.gop = it["got"].asInt();
        }
        if (it.isMember("fps") && it["fps"].isInt()) {
            video_params.fps = it["fps"].asFloat();
        }
        if (it.isMember("bitrate") && it["bitrate"].isInt()) {
            video_params.bitrate = it["bitrate"].asInt();
        }
        if (it.isMember("bitrate_type") && it["bitrate_type"].isString()) {
            std::string bitrate_type = it["bitrate_type"].asString();
            bitrate_type = toLowercase(bitrate_type);
            if (bitrate_type == "vbr") {
                video_params.rc_mode = VideoVBR;
            } else if (bitrate_type == "cbr") {
                video_params.rc_mode = VideoCBR;
            }
        }

        if (!hal::IVideo::instance()->setEncodeParams(channel, sub_channel, video_params)) {
            msg->code = 1003;
            msg->message = "hal error";
            return false;
        }
    }

    return true;
}

#endif

bool PrivHandler::get_video_config(MessagePtr &msg) {
    Json::Value data = Json::objectValue;
    if (IConfigManager::instance()->getConfig("video", data)) {
        msg->data = data;
        return true;
    }
    return false;
}

bool PrivHandler::set_video_config(MessagePtr &msg) {
    IConfigManager::ApplyResults results;
    bool r = IConfigManager::instance()->setConfig("video", msg->data, results);
    msg->data = Json::nullValue;
    return r;
}
