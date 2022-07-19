
#include "Log/Logger.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
constexpr int NN = 1e6;

void testFunc() {
    for (int i = 0; i < NN; i++) {
        auto curTID = std::this_thread::get_id();
        std::ostringstream ss;
        ss << curTID;
        size_t curId = std::stoull(ss.str().c_str());
        LOG_TRACE("TRACE Log, current thread id = %d\n", curId);
        LOG_DEBUG("DEBUG log, current thread id = %d\n", curId);
        LOG_INFO("INFO log, current thread id = %d\n", curId);
        LOG_WARN("WARN log, current thread id = %d\n", curId);
        LOG_ERROR("ERROR log, current thread id = %d\n", curId);
    }
}

void testGenerate() {
    /* 设置outPutFunc 和 flushFunc 为空, 测试日志行的生成速度 */
    Logger::setOutputFunc(nullptr);
    Logger::setFlushFunc(nullptr);
}


int main(int, char **) {
    auto start_time = std::chrono::system_clock::now();

    testGenerate();

    std::vector<std::thread> threads(1); 

    for (auto &t : threads)
        t = std::thread(testFunc);

    for (auto &t : threads)
        t.join();

    auto end_time = std::chrono::system_clock::now();
    std::chrono::nanoseconds cost_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    std::cout << "Log Line Generation Cost Time = " << cost_time.count() << " ns, Average Cost Time = " << static_cast<double>(cost_time.count()) / NN << " ns/item."<< std::endl;
    return 0;
}
