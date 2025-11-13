#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::string text = "Hello Hyperland!";

    // loop ตัวอักษรทีละตัว
    for (char c : text) {
        std::cout << c << std::flush;  // แสดงตัวอักษรทันที
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << std::endl; // ขึ้นบรรทัดใหม่
    return 0;
}
