#pragma once
#include <cstdlib>
#include <limits>
#include <string>
#include <type_traits>
#include "Timestamp.h"
#include "UnitBase.h"
#include "Logger.h"

namespace infra {

class Frequency final : public RelativeUnit<Frequency> {
public:
    template <typename T>
    static constexpr Frequency milliHertz(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromValue(value);
    }
    template <typename T>
    static constexpr Frequency hertz(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1'000, value);
    }
    template <typename T>
    static constexpr Frequency kiloHertz(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromFraction(1'000'000, value);
    }

    Frequency() = delete;

    template <typename T = int64_t>
    constexpr T hertz() const {
        return toFraction<1000, T>();
    }
    template <typename T = int64_t>
    constexpr T millihertz() const {
        return toValue<T>();
    }

 private:
    friend class UnitBase<Frequency>;
    using RelativeUnit::RelativeUnit;
    explicit Frequency(int64_t value);
    static constexpr bool one_sided = true;
};

inline Frequency operator/(int64_t nominator, const TimeDelta& interval) {
    constexpr int64_t kKiloPerMicro = 1000 * 1000000;
    //>LOG_CHECK_LE(nominator, std::numeric_limits<int64_t>::max() / kKiloPerMicro);
    LOG_CHECK(interval.IsFinite());
    LOG_CHECK(!interval.IsZero());
    return Frequency::milliHertz(nominator * kKiloPerMicro / interval.us());
}

inline TimeDelta operator/(int64_t nominator, const Frequency& frequency) {
    constexpr int64_t kMegaPerMilli = 1000000 * 1000;
    //>LOG_CHECK_LE(nominator, std::numeric_limits<int64_t>::max() / kMegaPerMilli);
    LOG_CHECK(frequency.isFinite());
    LOG_CHECK(!frequency.isZero());
    return TimeDelta::micros(nominator * kMegaPerMilli / frequency.millihertz());
}

inline double operator*(Frequency frequency, TimeDelta time_delta) {
    return frequency.hertz<double>() * time_delta.seconds<double>();
}
inline double operator*(TimeDelta time_delta, Frequency frequency) {
    return frequency * time_delta;
}

std::string ToString(Frequency value);
inline std::string ToLogString(Frequency value) {
    return ToString(value);
}

}  // namespace infra
