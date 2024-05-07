/********************************************************************
 * Copyright(c) 2024 ulucu technology
 * 
 * Author:     mengshunxiang
 * Date:       2024-02-23
 * Description: 时间戳
 * Others:
 *******************************************************************/
#pragma once
#include <stdint.h>
#include <cstddef>
#include <limits>
#include <algorithm>
#include "TimeDelta.h"
#include "NtpTime.h"
#include "Logger.h"

namespace infra {

static const int64_t kNumMillisecsPerSec = INT64_C(1000);
static const int64_t kNumMicrosecsPerSec = INT64_C(1000000);
static const int64_t kNumNanosecsPerSec = INT64_C(1000000000);

static const int64_t kNumMicrosecsPerMillisec = kNumMicrosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMillisec = kNumNanosecsPerSec / kNumMillisecsPerSec;
static const int64_t kNumNanosecsPerMicrosec = kNumNanosecsPerSec / kNumMicrosecsPerSec;

constexpr int64_t kNtpJan1970Millisecs = 2208988800 * kNumMillisecsPerSec;

static constexpr uint64_t kFractionsPerSecond = 0x100000000;

int64_t getCurrentTimeNs();
int64_t getCurrentTimeUs();
int64_t getCurrentTimeMs();


class Timestamp final : public UnitBase<Timestamp> {
public:
    template <typename T>
    static constexpr Timestamp seconds(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1000000, value);
    }
    template <typename T>
    static constexpr Timestamp millis(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1000, value);
    }
    template <typename T>
    static constexpr Timestamp micros(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromValue(value);
    }

    Timestamp() = delete;

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

    Timestamp operator+(const TimeDelta delta) const {
        if (isPlusInfinity() || delta.isPlusInfinity()) {
            LOG_CHECK(!isMinusInfinity());
            LOG_CHECK(!delta.isMinusInfinity());
            return plusInfinity();
        } else if (isMinusInfinity() || delta.isMinusInfinity()) {
            LOG_CHECK(!isPlusInfinity());
            LOG_CHECK(!delta.isPlusInfinity());
            return minusInfinity();
        }
        return Timestamp::micros(micros() + delta.micros());
    }
    Timestamp operator-(const TimeDelta delta) const {
        if (isPlusInfinity() || delta.isMinusInfinity()) {
            LOG_CHECK(!isMinusInfinity());
            LOG_CHECK(!delta.isPlusInfinity());
            return plusInfinity();
        } else if (isMinusInfinity() || delta.isPlusInfinity()) {
            LOG_CHECK(!isPlusInfinity());
            LOG_CHECK(!delta.isMinusInfinity());
            return minusInfinity();
        }
        return Timestamp::micros(micros() - delta.micros());
    }
    TimeDelta operator-(const Timestamp other) const {
        if (isPlusInfinity() || other.isMinusInfinity()) {
            LOG_CHECK(!isMinusInfinity());
            LOG_CHECK(!other.isPlusInfinity());
            return TimeDelta::plusInfinity();
        } else if (isMinusInfinity() || other.isPlusInfinity()) {
            LOG_CHECK(!isPlusInfinity());
            LOG_CHECK(!other.isMinusInfinity());
            return TimeDelta::minusInfinity();
        }
        return TimeDelta::micros(micros() - other.micros());
    }
    Timestamp& operator-=(const TimeDelta delta) {
        *this = *this - delta;
        return *this;
    }
    Timestamp& operator+=(const TimeDelta delta) {
        *this = *this + delta;
        return *this;
    }

    static Timestamp getCurrentTime() {
        return micros(getCurrentTimeUs());
    }

    static Timestamp now() {
        return micros(getCurrentTimeUs());
    }

private:
    friend class UnitBase<Timestamp>;
    using UnitBase::UnitBase;
    explicit Timestamp(int64_t value): UnitBase<Timestamp>(value) {};
    static constexpr bool one_sided = true;
};


inline uint32_t compactNtp(NtpTime ntp) {
    return (ntp.seconds() << 16) | (ntp.fractions() >> 16);
}

// January 1970, in NTP seconds.
const uint32_t kNtpJan1970 = 2208988800UL;
// Magic NTP fractional unit.
const double kMagicNtpFractionalUnit = 4.294967296E+9;

NtpTime convertTimestampToNtpTime(Timestamp timestamp);
TimeDelta compactNtpRttToTimeDelta(uint32_t compact_ntp_interval);

}