/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  reply.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 23:01:11
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Reply.h"
#include "MimeTypes.h"

const std::string ok =
        "HTTP/1.1 200 OK\r\n";
const std::string created =
        "HTTP/1.1 201 Created\r\n";
const std::string accepted =
        "HTTP/1.1 202 Accepted\r\n";
const std::string no_content =
        "HTTP/1.1 204 No Content\r\n";
const std::string multiple_choices =
        "HTTP/1.1 300 Multiple Choices\r\n";
const std::string moved_permanently =
        "HTTP/1.1 301 Moved Permanently\r\n";
const std::string moved_temporarily =
        "HTTP/1.1 302 Moved Temporarily\r\n";
const std::string not_modified =
        "HTTP/1.1 304 Not Modified\r\n";
const std::string bad_request =
        "HTTP/1.1 400 Bad Request\r\n";
const std::string unauthorized =
        "HTTP/1.1 401 Unauthorized\r\n";
const std::string forbidden =
        "HTTP/1.1 403 Forbidden\r\n";
const std::string not_found =
        "HTTP/1.1 404 Not Found\r\n";
const std::string internal_server_error =
        "HTTP/1.1 500 Internal Server Error\r\n";
const std::string not_implemented =
        "HTTP/1.1 501 Not Implemented\r\n";
const std::string bad_gateway =
        "HTTP/1.1 502 Bad Gateway\r\n";
const std::string service_unavailable =
        "HTTP/1.1 503 Service Unavailable\r\n";

std::string statusTypeToString(statusType status) {
    switch (status)
    {
    case statusType::ok:
        return ok;
    case statusType::created:
        return created;
    case statusType::accepted:
        return accepted;
    case statusType::no_content:
        return no_content;
    case statusType::multiple_choices:
        return multiple_choices;
    case statusType::moved_permanently:
        return moved_permanently;
    case statusType::moved_temporarily:
        return moved_temporarily;
    case statusType::not_modified:
        return not_modified;
    case statusType::bad_request:
        return bad_request;
    case statusType::unauthorized:
        return unauthorized;
    case statusType::forbidden:
        return forbidden;
    case statusType::not_found:
        return not_found;
    case statusType::internal_server_error:
        return internal_server_error;
    case statusType::not_implemented:
        return not_implemented;
    case statusType::bad_gateway:
        return bad_gateway;
    case statusType::service_unavailable:
        return service_unavailable;
    default:
        return internal_server_error;
    }
}

void Reply::addHeader(std::string& key, std::string& value) {
    headers_.emplace_back(key, value);
}

void Reply::addHeader(std::string key, std::string& value) {
    headers_.emplace_back(key, value);
}

void Reply::addHeader(std::string&& key, std::string&& value) {
    headers_.emplace_back(std::move(key), std::move(value));
}

void Reply::setContentType(std::string&& type) {
    addHeader("Content-type", mimeType(std::move(type)));
}

void Reply::setContent(std::string&& content) {
    addHeader("Content-Length", std::to_string(content.size()));
    body_type_ = ContentType::string;
    content_ = std::move(content);
}

void Reply::setStatus(statusType status) {
    status_ = status;
}

std::string Reply::toBuffer() {
    std::string response_content = statusTypeToString(status_);
    for (auto const& h : headers_) {
        response_content += h.first;
        response_content += ": ";
        response_content += h.second;
        response_content += "\r\n";
    }
    response_content += "\r\n";

    if (body_type_ == ContentType::string) {
        response_content += content_;
    }
    return response_content;
}

void Reply::reset() {
    status_ = statusType::init;
    headers_.clear();
    content_.clear();
}
