/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  HttpHandler.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-03 00:05:59
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "HttpHandler.h"
#include "infra/include/Logger.h"
#include "infra/include/File.h"
#include "MimeTypes.h"
#include "jsoncpp/include/value.h"

std::string HttpMethodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET:     return {"GET"};
        case HttpMethod::POST:    return {"POST"};
        case HttpMethod::PUT:     return {"PUT"};
        case HttpMethod::DELETE:  return {"DELETE"};
        case HttpMethod::PATCH:   return {"PATCH"};
        case HttpMethod::HEAD:    return {"HEAD"};
        case HttpMethod::OPTIONS: return {"OPTIONS"};
        case HttpMethod::TRACE:   return {"TRACE"};
        case HttpMethod::CONNECT: return {"CONNECT"};
        default: return {"GET"};
    }
}

void HttpHandler::config(IHttpServer::Config& config) {
    www_root_path_ = config.www_root_path;
    default_index_file_ = config.default_index_file;
}

bool HttpHandler::registerHandler(HttpMethod method, const char* url, httpHandler&& handler) {
    std::string method_str = HttpMethodToString(method);
    std::string key = method_str + std::string(url);
    std::lock_guard<std::mutex> guard(router_map_mutex_);
    auto it = router_map_.find(key);
    if (it != router_map_.end()) {
        errorf("%s had register\n", key.data());
        return false;
    }
    router_map_[key] = std::move(handler);
    return true;
}

bool HttpHandler::unRegisterHandler(HttpMethod method, const char* url, httpHandler&& handler) {
    std::string method_str = HttpMethodToString(method);
    std::string key = method_str + std::string(url);
    std::lock_guard<std::mutex> guard(router_map_mutex_);
    router_map_.erase(key);
    return true;
}

bool HttpHandler::isApiRequest(Request& request) {
    return request.url().find("/api") != std::string::npos;
}

std::string HttpHandler::getFileExtenssion(std::string& filename) {
    return filename.substr(filename.rfind("."), -1);
}

void HttpHandler::handleRequest(Request& request, Reply& reply) {
    infof("handleRequest %s %s\n", request.method().data(), request.url().data());

    if (isApiRequest(request)) {
        doApiRequest(request, reply);
        reply.addHeader("Content-type", mimeType(".json"));
        reply.setStatus(statusType::ok);
    } else {
        // get file
        std::string root_path = www_root_path_;
        std::string file_path = request.url();
        if (file_path == "/") {
            reply.setStatus(statusType::moved_permanently);
            reply.addHeader("Location", default_index_file_);
            return;
        }

        std::string content = infra::File::loadFile(root_path + file_path);
        if (content.length()) {
            std::string extension = getFileExtenssion(file_path);
            reply.setContentType(std::move(extension));
            reply.setContent(std::move(content));
            reply.setStatus(statusType::ok);
        } else {
            reply.setStatus(statusType::not_found);
        }
    }
}

void HttpHandler::doApiRequest(Request& request, Reply& reply) {
    std::string key = request.method() + request.url();
    router_map_mutex_.lock();
    auto it = router_map_.find(key);
    if (it == router_map_.end()) {
        router_map_mutex_.unlock();
        warnf("not found %s %s\n", request.method().data(), request.url().data());




        Json::Value data = Json::nullValue;
        data["code"] = -1000;
        data["message"] = "not found handler:" + request.method() + " " + request.url();
        //data["data"] = Json::nullValue;

        std::string reaponse_body = data.toStyledString();
        reply.setContent(std::move(reaponse_body));
        return;
    }
    router_map_mutex_.unlock();


    std::string reaponse_body;
    it->second(request.queries(), request.body(), reaponse_body);
    reply.setContent(std::move(reaponse_body));
}