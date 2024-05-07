/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Request.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-02 21:50:42
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "Request.h"
#include "infra/include/Logger.h"
#include "http/common/utils.h"

Request::Request() : buffer_(8 * 1024) {
}

Request::~Request() {
}

int32_t Request::readRequest(std::shared_ptr<infra::TcpSocket> &sock) {
    int32_t left_size = buffer_.leftSize() - 1;
    char *recv_buffer = buffer_.data() + buffer_.size();

    int32_t ret = sock->recv(recv_buffer, left_size);
    if (ret > 0) {
        recv_buffer[ret] = 0;
        buffer_.setSize(buffer_.size() + ret);
        
        if (ret >= left_size) {
            const int32_t min_size = 1024 * 10;
            int32_t new_size = std::min(buffer_.capacity() * 2, min_size);
            buffer_.ensureCapacity(new_size);

            warnf("buffer is too small, need read again, buffer expand to %d\n", new_size);
            left_size = buffer_.leftSize();
            recv_buffer = buffer_.data() + buffer_.size() - 1;

            ret = sock->recv(recv_buffer, left_size);
            recv_buffer[ret] = 0;
            buffer_.setSize(buffer_.size() + ret);
        }
        tracef("recv http request length:%d \n%s\n", buffer_.size(), buffer_.data());
    }
    return ret;
}

bool Request::parse() {
    const char *method = nullptr;
    size_t method_len = 0;
    const char* path = nullptr;
    size_t path_len = 0;
    size_t last_len = 0;
    headers_num_ = sizeof(headers_) / sizeof(headers_[0]);
    header_len_ = phr_parse_request((const char*)buffer_.data(), buffer_.size(), &method, &method_len, &path, &path_len, 
        &minor_version_, headers_, &headers_num_, last_len);
    if (header_len_ < 0) {
        errorf("http_parser failed header_len:%d\n", header_len_);
        return false;
    }
    method_ = std::string(method, method_len);
    url_ = std::string(path, path_len);

    auto header_value = getHeaderValue("content-length");
    if (header_value.empty()) {
        auto transfer_encoding = getHeaderValue("transfer-encoding");
        if (transfer_encoding == "chunked") {
            is_chunked_ = true;
        }
        body_len_ = 0;
    } else {
        body_len_ = atoll(header_value.data());
        body_str_ = buffer_.data() + header_len_;
    }

    size_t pos = url_.find('?');
    if (pos != std::string::npos) {
        parseQuery(url_.substr(pos + 1, url_.length() - pos - 1));
    }
    return true;
}

void Request::parseQuery(std::string str) {
    std::string key;
    std::string val;
    size_t pos = 0;
    size_t length = str.length();
    for (size_t i = 0; i < length; i++) {
        char c = str[i];
        if (c == '=') {
            key = { &str[pos], i - pos };
            key = trimSpace(key);
            pos = i + 1;
        } else if (c == '&') {
            val = { &str[pos], i - pos };
            val = trimSpace(val);
            pos = i + 1;
            //if (is_form_url_encode(key)) {
            //	auto s = form_urldecode(key);
            //}
            queries_.emplace(key, val);
        }
    }

    if ((length - pos) > 0) {
        val = { &str[pos], length - pos };
        val = trimSpace(val);
        queries_.emplace(key, val);
    } else if ((length - pos) == 0) {
        queries_.emplace(key, "");
    }
}

std::string Request::method() const {
    return method_;
}

std::string Request::url() const {
    return url_;
}

std::string Request::getHeaderValue(std::string key) const {
	for (size_t i = 0; i < headers_num_; i++) {
		if (iequal(headers_[i].name, headers_[i].name_len, key.data())) {
			return std::string(headers_[i].value, headers_[i].value_len);
		}
	}
	return {};
}

std::string Request::getQueryValue(std::string key) {
    std::string url = url_;
    url = (url.length() > 1 && url.back() == '/') ? url.substr(0, url.length() - 1) : url;
    std::string map_key = url.append(key);

    /*auto it = queries_.find(key);
    if (it == queries_.end()) {
        auto form_it = form_url_map_.find(key);
        if (form_it == form_url_map_.end()) {
            return {};
        }

        if (code_utils::is_url_encode(form_it->second)) {
            auto ret= utf8_character_params_.emplace(map_key, code_utils::get_string_by_urldecode(form_it->second));
            return ret.first->second;
        }
        return form_it->second;
    }

    if (code_utils::is_url_encode(it->second)) {
        auto ret = utf8_character_params_.emplace(map_key, code_utils::get_string_by_urldecode(it->second));
        return ret.first->second;
    }
    return it->second;
    */
    return {};
}

bool Request::isHttp11() {
	return minor_version_ == 1;
}

void Request::reset() {
    buffer_.resize(0);
    is_chunked_ = false;
    queries_.clear();
}