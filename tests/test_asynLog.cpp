#include "Log/AsynLog.h"
#include "Log/FileWriter.h"
#include "Log/LogConfig.h"
#include "Log/Logger.h"
#include "Thread/ThreadInfo.h"
#include <cstddef>
#include <memory>
#include <sstream>
#include <thread>
#include <iostream>

const int NN = 1e6;

void testFunc() {
    for (int i = 0; i < NN; i++) {
        auto curTid = std::this_thread::get_id();
        LOG_TRACE("TRACE Log, current thread id = %s\n", curTid);
        LOG_DEBUG("DEBUG log, current thread id = %s\n", curTid);
        LOG_INFO("INFO log, current thread id = %d\n", curTid);
        LOG_WARN("WARN log, current thread id = %d\n", curTid);
        LOG_ERROR("ERROR log, current thread id = %d\n", curTid);
    }
}
std::unique_ptr<AsynLog> asynLog;

void asynOutput(const char *msg, size_t len, size_t keyLen = 0) {
    asynLog->append(msg, len);
}

void AsynLogger() {
    LogConfig config;
    config.logLevel = LogConfig::INFO;
    config.logFileOptions.baseName = "test-AsynLog";
    config.logFileOptions.fileWriterType = FileWriterType::FileWriter_NORMAL;
    config.logFileOptions.rollSize = static_cast<size_t>(500 * 1024 * 1024);
    Logger::setConfig(config);
    Logger::setOutputFunc(asynOutput);
    asynLog = std::make_unique<AsynLog>();
    asynLog->start();
}

int main() {
    auto start_time = std::chrono::system_clock::now();

    AsynLogger();

    std::vector<std::thread> threads(8);

    for (auto &t : threads)
        t = std::thread(testFunc);

    for (auto &t : threads)
        t.join();

    auto end_time = std::chrono::system_clock::now();
    std::chrono::milliseconds cost_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "AsynLogger cost time = " << cost_time.count() << " ms" << std::endl;
    return 0;
}