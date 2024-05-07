#include "logger.h"
#include <stdio.h>
#include <thread>
#include <unistd.h>
#include "optional.h"
#include <optional>

#include "network/Pipe.h"

#define print_log_count 2

void logTestThread1() {
    int32_t count = 0;
    while (true) {
        errorf("count:%d\n", count++);
        if (count > print_log_count) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main(int argc, char* argv[]) {

    std::shared_ptr<infra::LogChannel> console_log = std::make_shared<infra::ConsoleLogChannel>();
    infra::Logger::instance().addLogChannel(console_log);

    std::shared_ptr<infra::LogChannel> file_log = std::make_shared<infra::FileLogChannel>("log.log");
    infra::Logger::instance().addLogChannel(file_log);

    errorf("test\n");
    warnf("test\n");
    infof("test\n");
    debugf("test\n");
    tracef("test\n");

    infra::Pipe pipe;

    infra::optional<bool> is_start = {true};

    tracef("is_start.has_value %d\n", is_start.has_value() ? 1 : 0);
    if (is_start.has_value()) {
        tracef("is_start.value %d\n", *is_start ? 1 : 0);
    }

    char* tty_name = ttyname(STDOUT_FILENO);
    infof("tty_name:%s\n", tty_name);

    std::thread thread1([]() {logTestThread1();});

    //std::nullopt;

    int32_t count = 0;
    while (true) {
        tracef("count:%d\n", count++);
        if (count > print_log_count) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    thread1.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    exit(0);
    return 0;
}
