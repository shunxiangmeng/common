#pragma once
#include <limits>
#include <string>
#include <type_traits>
#include "DataSize.h"
#include "Timestamp.h"
#include "Logger.h"
#include "Frequency.h"

namespace infra {

class DataRate final : public RelativeUnit<DataRate> {
public:
    template <typename T>
    static constexpr DataRate bitsPerSec(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromValue(value);
    }
    template <typename T>
    static constexpr DataRate bytesPerSec(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(8, value);
    }
    template <typename T>
    static constexpr DataRate kilobitsPerSec(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1000, value);
    }
    static DataRate infinity() { return plusInfinity(); }

    DataRate() = delete;

    template <typename T = int64_t>
    constexpr T bitsPerSec() const {
        return toValue<T>();
    }
    template <typename T = int64_t>
    constexpr T bps() const {
        return toValue<T>();
    }

    template <typename T = int64_t>
    constexpr T bytesPerSec() const {
        return toFraction<8, T>();
    }

    template <typename T = int64_t>
    constexpr T kilobitsPerSec() const {
        return toFraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T kbps() const {
        return toFraction<1000, T>();
    }

    constexpr int64_t bps_or(int64_t fallback_value) const {
        return toValueOr(fallback_value);
    }
    constexpr int64_t kbps_or(int64_t fallback_value) const {
        return toFractionOr<1000>(fallback_value);
    }

private:
    // Bits per second used internally to simplify debugging by making the value
    // more recognizable.
    friend class UnitBase<DataRate>;
    using RelativeUnit::RelativeUnit;
    explicit DataRate(int64_t value): RelativeUnit<DataRate>(value) {};
    static constexpr bool one_sided = true;
};

namespace data_rate_impl {
inline int64_t microbits(const DataSize& size) {
    constexpr int64_t kMaxBeforeConversion = std::numeric_limits<int64_t>::max() / 8000000;
    LOG_CHECK_LE(size.bytes(), kMaxBeforeConversion);
    return size.bytes() * 8000000;
}

inline int64_t millibytePerSec(const DataRate& size) {
    constexpr int64_t kMaxBeforeConversion = std::numeric_limits<int64_t>::max() / (1000 / 8);
    LOG_CHECK_LE(size.bps(), kMaxBeforeConversion);
    return size.bps() * (1000 / 8);
}
}  // namespace data_rate_impl

inline DataRate operator/(const DataSize size, const TimeDelta duration) {
    return DataRate::bitsPerSec(data_rate_impl::microbits(size) / duration.us());
}
inline TimeDelta operator/(const DataSize size, const DataRate rate) {
    return TimeDelta::micros(data_rate_impl::microbits(size) / rate.bps());
}
inline DataSize operator*(const DataRate rate, const TimeDelta duration) {
    int64_t microbits = rate.bps() * duration.us();
    return DataSize::bytes((microbits + 4000000) / 8000000);
}
inline DataSize operator*(const TimeDelta duration, const DataRate rate) {
    return rate * duration;
}

inline DataSize operator/(const DataRate rate, const Frequency frequency) {
    int64_t millihertz = frequency.millihertz<int64_t>();
    // Note that the value is truncated here reather than rounded, potentially
    // introducing an error of .5 bytes if rounding were expected.
    return DataSize::bytes(data_rate_impl::millibytePerSec(rate) / millihertz);
}
inline Frequency operator/(const DataRate rate, const DataSize size) {
    return Frequency::milliHertz(data_rate_impl::millibytePerSec(rate) / size.bytes());
}
inline DataRate operator*(const DataSize size, const Frequency frequency) {
    //>LOG_CHECK(frequency.IsZero() || size.bytes() <= std::numeric_limits<int64_t>::max() / 8 / frequency.millihertz<int64_t>());
    int64_t millibits_per_second = size.bytes() * 8 * frequency.millihertz<int64_t>();
    return DataRate::bitsPerSec((millibits_per_second + 500) / 1000);
}
inline DataRate operator*(const Frequency frequency, const DataSize size) {
    return size * frequency;
}

std::string ToString(DataRate value);
inline std::string ToLogString(DataRate value) {
    return ToString(value);
}

}  // namespace infra

