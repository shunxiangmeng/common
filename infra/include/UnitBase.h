/********************************************************************
 * Copyright(c) 2024 ulucu technology
 * 
 * Author:     mengshunxiang
 * Date:       2024-01-22
 * Description: Unit基类【参考google webrtc代码】
 * Others:
 *******************************************************************/
#pragma once
#include <stdint.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include "DivideRound.h"
#include "Logger.h"

namespace infra {

template <class Unit_T>
class UnitBase {
public:
    UnitBase() = delete;
    static constexpr Unit_T zero() { return Unit_T(0); }
    static constexpr Unit_T plusInfinity() { return Unit_T(plusInfinityVal()); }
    static constexpr Unit_T minusInfinity() { return Unit_T(minusInfinityVal()); }

    constexpr bool isZero() const { return value_ == 0; }
    constexpr bool isFinite() const { return !isInfinite(); }
    constexpr bool isInfinite() const {
        return value_ == plusInfinityVal() || value_ == minusInfinityVal();
    }
    constexpr bool isPlusInfinity() const { return value_ == plusInfinityVal(); }
    constexpr bool isMinusInfinity() const {
        return value_ == minusInfinityVal();
    }

    constexpr bool operator==(const UnitBase<Unit_T>& other) const {
        return value_ == other.value_;
    }
    constexpr bool operator!=(const UnitBase<Unit_T>& other) const {
        return value_ != other.value_;
    }
    constexpr bool operator<=(const UnitBase<Unit_T>& other) const {
        return value_ <= other.value_;
    }
    constexpr bool operator>=(const UnitBase<Unit_T>& other) const {
        return value_ >= other.value_;
    }
    constexpr bool operator>(const UnitBase<Unit_T>& other) const {
        return value_ > other.value_;
    }
    constexpr bool operator<(const UnitBase<Unit_T>& other) const {
        return value_ < other.value_;
    }

    constexpr Unit_T roundTo(const Unit_T& resolution) const {
        LOG_CHECK(isFinite());
        LOG_CHECK(resolution.isFinite());
        LOG_CHECK_GT(resolution.value_, 0);
        return Unit_T((value_ + resolution.value_ / 2) / resolution.value_) * resolution.value_;
    }
    constexpr Unit_T roundUpTo(const Unit_T& resolution) const {
        LOG_CHECK(isFinite());
        LOG_CHECK(resolution.isFinite());
        LOG_CHECK_GT(resolution.value_, 0);
        return Unit_T((value_ + resolution.value_ - 1) / resolution.value_) * resolution.value_;
    }
    constexpr Unit_T roundDownTo(const Unit_T& resolution) const {
        LOG_CHECK(isFinite());
        LOG_CHECK(resolution.isFinite());
        LOG_CHECK_GT(resolution.value_, 0);
        return Unit_T(value_ / resolution.value_) * resolution.value_;
    }

protected:
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    static constexpr Unit_T fromValue(T value) {
        if (Unit_T::one_sided) {
            LOG_CHECK_GE(value, 0);
        }
        return Unit_T(static_cast<int64_t>(value));
    }
    
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    static constexpr Unit_T fromValue(T value) {
        if (value == std::numeric_limits<T>::infinity()) {
            return plusInfinity();
        } else if (value == -std::numeric_limits<T>::infinity()) {
            return minusInfinity();
        } else {
            LOG_CHECK(!std::isnan(value));
            return fromValue(static_cast<int64_t>(value));
        }
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    static constexpr Unit_T fromFraction(int64_t denominator, T value) {
        if (Unit_T::one_sided) {
            LOG_CHECK_GE(value, 0);
        }
        LOG_CHECK_GT(value, minusInfinityVal() / denominator);
        LOG_CHECK_LT(value, plusInfinityVal() / denominator);
        return Unit_T(static_cast<int64_t>(value * denominator));
    }
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    static constexpr Unit_T fromFraction(int64_t denominator, T value) {
        return fromValue(value * denominator);
    }

    template <typename T = int64_t>
    constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
    toValue() const {
        LOG_CHECK(isFinite());
        return static_cast<T>(value_);
    }
    template <typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
    toValue() const {
        return isPlusInfinity()
                ? std::numeric_limits<T>::infinity()
                : isMinusInfinity() ? -std::numeric_limits<T>::infinity() : value_;
    }
    template <typename T>
    constexpr T toValueOr(T fallback_value) const {
        return isFinite() ? value_ : fallback_value;
    }

    template <int64_t Denominator, typename T = int64_t>
    constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
    toFraction() const {
        //LOG_CHECK(IsFinite());
        return static_cast<T>(divideRoundToNearest(value_, Denominator));
    }
    template <int64_t Denominator, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
    toFraction() const {
        return toValue<T>() * (1 / static_cast<T>(Denominator));
    }

    template <int64_t Denominator>
    constexpr int64_t toFractionOr(int64_t fallback_value) const {
        return isFinite() ? divideRoundToNearest(value_, Denominator) : fallback_value;
    }

    template <int64_t Factor, typename T = int64_t>
    constexpr typename std::enable_if<std::is_integral<T>::value, T>::type
    toMultiple() const {
        LOG_CHECK_GE(toValue(), std::numeric_limits<T>::min() / Factor);
        LOG_CHECK_LE(toValue(), std::numeric_limits<T>::max() / Factor);
        return static_cast<T>(toValue() * Factor);
    }
    template <int64_t Factor, typename T>
    constexpr typename std::enable_if<std::is_floating_point<T>::value, T>::type
    toMultiple() const {
        return toValue<T>() * Factor;
    }

    explicit UnitBase(int64_t value) : value_(value) {};
private:
    template <class RelativeUnit_T>
    friend class RelativeUnit;

    static inline constexpr int64_t plusInfinityVal() {
        return std::numeric_limits<int64_t>::max();
    }
    static inline constexpr int64_t minusInfinityVal() {
        return std::numeric_limits<int64_t>::min();
    }
    constexpr Unit_T& asSubClassRef() { return static_cast<Unit_T&>(*this); }
    constexpr const Unit_T& asSubClassRef() const {
        return static_cast<const Unit_T&>(*this);
    }

    int64_t value_;
};

template <class Unit_T>
class RelativeUnit : public UnitBase<Unit_T> {
public:
    constexpr Unit_T clamped(Unit_T min_value, Unit_T max_value) const {
        return std::max(min_value, std::min(UnitBase<Unit_T>::asSubClassRef(), max_value));
    }
    constexpr void clamp(Unit_T min_value, Unit_T max_value) {
        *this = clamped(min_value, max_value);
    }
    constexpr Unit_T operator+(const Unit_T other) const {
        if (this->isPlusInfinity() || other.isPlusInfinity()) {
            LOG_CHECK(!this->isMinusInfinity());
            LOG_CHECK(!other.isMinusInfinity());
            return this->plusInfinity();
        } else if (this->isMinusInfinity() || other.isMinusInfinity()) {
            LOG_CHECK(!this->isPlusInfinity());
            LOG_CHECK(!other.isPlusInfinity());
            return this->minusInfinity();
        }
        return UnitBase<Unit_T>::fromValue(this->toValue() + other.toValue());
    }
    constexpr Unit_T operator-(const Unit_T other) const {
        if (this->isPlusInfinity() || other.isMinusInfinity()) {
            LOG_CHECK(!this->isMinusInfinity());
            LOG_CHECK(!other.isPlusInfinity());
            return this->plusInfinity();
        } else if (this->isMinusInfinity() || other.isPlusInfinity()) {
            LOG_CHECK(!this->isPlusInfinity());
            LOG_CHECK(!other.isMinusInfinity());
            return this->minusInfinity();
        }
        return UnitBase<Unit_T>::fromValue(this->toValue() - other.toValue());
    }
    constexpr Unit_T& operator+=(const Unit_T other) {
        *this = *this + other;
        return this->asSubClassRef();
    }
    constexpr Unit_T& operator-=(const Unit_T other) {
        *this = *this - other;
        return this->asSubClassRef();
    }
    constexpr double operator/(const Unit_T other) const {
        return UnitBase<Unit_T>::template toValue<double>() / other.template toValue<double>();
    }
    // std::enable_if_t 是 C++14 的特性, C++11 的特性是 std::enable_if
    template <typename T, typename std::enable_if_t<std::is_floating_point<T>::value> * = nullptr>
    constexpr Unit_T operator/(T scalar) const {
        return UnitBase<Unit_T>::fromValue(std::llround(this->toValue() / scalar));
    }
    template <typename T, typename std::enable_if_t<std::is_integral<T>::value> * = nullptr>
    constexpr Unit_T operator/(T scalar) const {
        return UnitBase<Unit_T>::fromValue(this->toValue() / scalar);
    }
    constexpr Unit_T operator*(double scalar) const {
        return UnitBase<Unit_T>::fromValue(std::llround(this->toValue() * scalar));
    }
    constexpr Unit_T operator*(int64_t scalar) const {
        return UnitBase<Unit_T>::fromValue(this->toValue() * scalar);
    }
    constexpr Unit_T operator*(int32_t scalar) const {
        return UnitBase<Unit_T>::fromValue(this->toValue() * scalar);
    }
    constexpr Unit_T operator*(size_t scalar) const {
        return UnitBase<Unit_T>::fromValue(this->toValue() * scalar);
    }

protected:
    using UnitBase<Unit_T>::UnitBase;
};

template <class Unit_T>
inline constexpr Unit_T operator*(double scalar, RelativeUnit<Unit_T> other) {
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int64_t scalar, RelativeUnit<Unit_T> other) {
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(int32_t scalar, RelativeUnit<Unit_T> other) {
    return other * scalar;
}
template <class Unit_T>
inline constexpr Unit_T operator*(size_t scalar, RelativeUnit<Unit_T> other) {
    return other * scalar;
}

template <class Unit_T>
inline constexpr Unit_T operator-(RelativeUnit<Unit_T> other) {
    if (other.isPlusInfinity()) {
        return UnitBase<Unit_T>::minusInfinity();
    }
    if (other.isMinusInfinity()) {
        return UnitBase<Unit_T>::plusInfinity();
    }
    return -1 * other;
}

}