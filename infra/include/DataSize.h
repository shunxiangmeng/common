#pragma once
#include <stdint.h>
#include "UnitBase.h"

namespace infra {
class DataSize final : public RelativeUnit<DataSize> {
public:
    template <typename T>
    static constexpr DataSize bytes(T value) {
        static_assert(std::is_arithmetic<T>::value, "");
        return fromValue(value);
    }
    static DataSize infinity() { return plusInfinity(); }

    DataSize() = delete;

    template <typename T = int64_t>
    constexpr T bytes() const {
        return toValue<T>();
    }

    constexpr int64_t bytes_or(int64_t fallback_value) const {
        return toValueOr(fallback_value);
    }

private:
    friend class UnitBase<DataSize>;
    using RelativeUnit::RelativeUnit;
    explicit DataSize(int64_t value): RelativeUnit<DataSize>(value) {};
    static constexpr bool one_sided = true;
};
}
