#pragma once
#include <stdint.h>
#include <cstddef>
#include <limits>
#include <algorithm>
#include "UnitBase.h"

namespace infra {

class TimeDelta final : public RelativeUnit<TimeDelta> {
public:
    template <typename T>
    static constexpr TimeDelta minutes(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return seconds(value * 60);
    }
    template <typename T>
    static constexpr TimeDelta seconds(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1000000, value);
    }
    template <typename T>
    static constexpr TimeDelta millis(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1000, value);
    }
    template <typename T>
    static constexpr TimeDelta micros(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromValue(value);
    }

    TimeDelta() = delete;

    template <typename T = int64_t>
    constexpr T seconds() const {
        return toFraction<1000000, T>();
    }
    template <typename T = int64_t>
    constexpr T millis() const {
        return toFraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T micros() const {
        return toValue<T>();
    }
    template <typename T = int64_t>
    constexpr T ns() const {
        return toMultiple<1000, T>();
    }
    TimeDelta abs() const {
        return micros() < 0 ? TimeDelta::micros(-micros()) : *this;
    }
private:
    friend class UnitBase<TimeDelta>;
    using RelativeUnit::RelativeUnit;
    explicit TimeDelta(int64_t value): RelativeUnit<TimeDelta>(value) {};
    static constexpr bool one_sided = false;
};

}