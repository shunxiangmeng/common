#pragma once
#include <cstdint>
#include <future>

enum class result_code : std::int16_t {
    OK = 0,
    FAIL = 1,
};

enum class error_code {
    OK,
    UNKNOWN,
    FAIL,
    TIMEOUT,
    CANCEL,
    BADCONNECTION,
};

enum class request_type : uint32_t { req_res, sub_pub };

struct message_type {
    std::uint64_t req_id;
    request_type req_type;
    std::shared_ptr<std::string> content;
};

static const uint8_t MAGIC_NUM = 39;
#define MAGIC_RPC     0x2A2A2A2A  //****

#pragma pack(1)
struct rpc_header {
    uint32_t magic;     //MAGIC_RPC
    uint32_t body_len;
    request_type req_type;
    uint32_t req_id;
    uint32_t func_id;
};
#pragma pack()

static const size_t MAX_BUF_LEN = 1048576 * 10;
static const size_t HEAD_LEN = sizeof(rpc_header);
static const size_t INIT_BUF_SIZE = 2 * 1024;


enum class CallModel { future, callback };
const constexpr auto FUTURE = CallModel::future;
const constexpr size_t DEFAULT_TIMEOUT = 1000; // milliseconds

inline bool has_error(std::string result) {
    if (result.empty()) {
        return true;
    }
    msgpack_codec codec;
    auto tp = codec.unpack<std::tuple<int>>(result.data(), result.size());
    return std::get<0>(tp) != 0;
}

inline std::string get_error_msg(std::string result) {
    msgpack_codec codec;
    auto tp = codec.unpack<std::tuple<int, std::string>>(result.data(), result.size());
    return std::get<1>(tp);
}

typedef enum {
    req_success = 0,
    req_send_failed,
    req_timeout
} req_error_code;

class req_result {
public:
    req_result() = default;
    req_result(req_error_code code, std::string data) : error_code_(code), data_(data) {}
    
    template <typename T> T as() {
        if (!success()) {
            std::string err_msg = data_.empty() ? data_ : get_error_msg(data_);
            //throw std::logic_error(err_msg);
            return T();
        }
        return get_result<T>(data_);
    }

    void as() {
        if (has_error(data_)) {
            std::string err_msg = data_.empty() ? data_ : get_error_msg(data_);
            throw std::logic_error(err_msg);
        }
    }

    bool success() const { return error_code_ == req_success; }

private:
    req_error_code error_code_;
    std::string data_;
};

template <typename T> 
struct future_result {
    uint64_t id;
    std::future<T> future;
    template <class Rep, class Per>
    std::future_status wait_for(const std::chrono::duration<Rep, Per> &rel_time) {
        return future.wait_for(rel_time);
    }
    T get() { return future.get(); }
};