#include <chrono>
#include <algorithm>
#include "infra/include/Timestamp.h"
#include "infra/include/NtpTime.h"

namespace infra {

int64_t getCurrentTimeNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t getCurrentTimeUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string getCurrentTime() {
    //auto now = std::chrono::high_resolution_clock::now();
    //std::time_t now_c = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    //return std::ctime(&now_c);
    time_t now = time(NULL);
    struct tm* local;
    //local = std::gmtime(&now);  utc time
    local = localtime(&now);
    char time_str[128];
    size_t size = strftime(time_str, sizeof(time_str), "%Y-%m-%d %X", local);
    return std::string(time_str);
}

NtpTime convertTimestampToNtpTime(Timestamp timestamp) {
    int64_t now_us = timestamp.micros();
    uint32_t seconds = (uint32_t)(now_us / 1000000) + kNtpJan1970;
    uint32_t fractions = static_cast<uint32_t>((now_us % 1000000) * kMagicNtpFractionalUnit / 1000000);
    return NtpTime(seconds, fractions);
}

TimeDelta compactNtpRttToTimeDelta(uint32_t compact_ntp_interval) {
    static TimeDelta kMinRtt = TimeDelta::millis(1);
    // Interval to convert expected to be positive, e.g. RTT or delay.
    // Because interval can be derived from non-monotonic ntp clock,
    // it might become negative that is indistinguishable from very large values.
    // Since very large RTT/delay is less likely than non-monotonic ntp clock,
    // such value is considered negative and converted to minimum value of 1ms.
    if (compact_ntp_interval > 0x80000000) {
        return kMinRtt;
    }
    // Convert to 64bit value to avoid multiplication overflow.
    int64_t value = static_cast<int64_t>(compact_ntp_interval);
    // To convert to TimeDelta need to divide by 2^16 to get seconds,
    // then multiply by 1'000'000 to get microseconds. To avoid float operations,
    // multiplication and division are swapped.
    int64_t us = divideRoundToNearest(value * kNumMicrosecsPerSec, 1 << 16);
    // Small RTT value is considered too good to be true and increased to 1ms.
    return std::max(TimeDelta::micros(us), kMinRtt);
}

}