/********************************************************************
 * Copyright(c) 2024 ulucu technology
 * 
 * Author:     mengshunxiang
 * Date:       2024-02-19
 * Description: std::optional 是 C++17 的特性，由于很多嵌入式的 gcc 版本
 *              比较低还不支持C++17, 所以用 C++11 实现一个 optional 模板
 * Others:
 *******************************************************************/
#pragma once
#include <type_traits>
#include <iostream>
#include <string>
#include <map>

namespace infra {

template<typename T>
class optional {
    using data_t = typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type;
public:
    optional() : has_init_(false) {}
    optional(const T &value) {
        create(value);
    }
    optional(T&& value) {
        has_init_ = false;
        create(std::move(value));
    }
    ~optional() {
        destory();
    }

    optional(const optional& other) : has_init_(false) {
        if (other.init()) {
            assing(other);
        }
    }
    optional(optional&& other) : has_init_(false) {
        if (other.init()) {
            assign(std::move(other));
            other.destory();
        }
    }
    optional& operator=(optional &&other) {
        assign(std::move(other));
        return *this;
    }
    optional& operator=(const optional &other) {
        assign(std::move(other));
        return *this;  
    }
    template<class... Args>
    void emplace(Args&&... args) {
        destory();
        create(std::forward<Args>(args)...);
    }

    bool init() const {
        return has_init_;
    }

    bool has_value() const {
        return has_init_;
    }

    explicit operator bool() const {
        return init();
    }

    T& operator*() {
        if (init()) {
            return *((T*)(&data_));
        }
        throw std::logic_error("infra optional is not init");
    }

    T const& operator*() const {
        if (init()) {
            return *((T*)(&data_));
        }
        throw std::logic_error("infra optional is not init");
    }

    bool operator == (const optional<T>& other) const {
        return (!bool(*this)) != (!other) ? false : (!bool(*this) ? true : (*(*this)) == (*other));
    }

    bool operator < (const optional<T>& other) const {
        return !other ? false : (!bool(*this) ? true : (*(*this) < (*other)));
    }

    bool operator != (const optional<T>& other) {
        return !(*this == (other));
    }

private:
    template<class... Args>
    void create(Args&&... args) {
        new (&data_) T(std::forward<Args>(args)...);
        has_init_ = true;
    }

    void destory() {
        if (has_init_) {
            has_init_ = false;
            ((T*)(&data_))->~T();
        }
    }

    void assign(const optional& other) {
        if (other.init()) {
            copy(other.data_);
            has_init_ = true;
        } else {
            destory();
        }
    }

    void assign(optional&& other) {
        if (other.init()) {
            move(std::move(other.data_));
            has_init_ = true;
            other.destory();
        } else {
            destory();
        }
    }

    void move(data_t&& val) {
        destory();
        new (&data_) T(std::move(*((T*)(&val))));
    }

    void copy(const data_t& val) {
        destory();
        new (&data_) T(*((T*)(&val)));
    }

private:
    bool has_init_;
    data_t data_;
};
}
