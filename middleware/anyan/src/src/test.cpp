#include <chrono>
#include <thread>
#include "Anyan_Device_SDK.h"

int main() {
    Dev_SN_Info oem_info{0};
    Dev_Attribut_Struct devattr{0};
    Ulu_SDK_Init_All(&oem_info, &devattr, nullptr);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}