/********************************************************************
 * Copyright(c) 2024 ulucu technology
 * 
 * Author:     mengshunxiang
 * Date:       2024-02-23
 * Description: 浮点数除法取整
 * Others:
 *******************************************************************/
#pragma once
#include <type_traits>
#include "Logger.h"

namespace infra {

template <typename Dividend, typename Divisor>
inline auto constexpr divideRoundUp(Dividend dividend, Divisor divisor) {
    static_assert(std::is_integral<Dividend>(), "");
    static_assert(std::is_integral<Divisor>(), "");
    LOG_CHECK_GE(dividend, 0);
    LOG_CHECK_GT(divisor, 0);
    auto quotient = dividend / divisor;
    auto remainder = dividend % divisor;
    return quotient + (remainder > 0 ? 1 : 0);
}

template <typename Dividend, typename Divisor>
inline auto constexpr divideRoundToNearest(Dividend dividend, Divisor divisor) {
    static_assert(std::is_integral<Dividend>(), "");
    static_assert(std::is_integral<Divisor>(), "");
    LOG_CHECK_GT(divisor, 0);

    if (dividend < Dividend{0}) {
        auto half_of_divisor = divisor / 2;
        auto quotient = dividend / divisor;
        auto remainder = dividend % divisor;
        if (-remainder > half_of_divisor) {
            --quotient;
        }
        return quotient;
    }

    auto half_of_divisor = (divisor - 1) / 2;
    auto quotient = dividend / divisor;
    auto remainder = dividend % divisor;
    if (remainder > half_of_divisor) {
        ++quotient;
    }
    return quotient;
}

}  // namespace infra