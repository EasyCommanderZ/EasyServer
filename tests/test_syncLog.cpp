#include "Log/FileWriter.h"
#include "Log/LogConfig.h"
#include "Log/LogFile.h"
#include "Log/Logger.h"
#include "Log/SynLog.h"
#include "Thread/ThreadInfo.h"
#include <memory>
#include <thread>
#include <cstddef>
#include <iostream>
#include <chrono>
#include <vector>
#include <sstream>
#include <string>

const int NN = 1e6;

void testFunc() {
    for (int i = 0; i < NN; i++) {
        const char *curTid = ThreadInfo::tidString();
        LOG_TRACE("TRACE Log, current thread id = %s\n", curTid);
        LOG_DEBUG("DEBUG log, current thread id = %s\n", curTid);
        LOG_INFO("INFO log, current thread id = %d\n", curTid);
        LOG_WARN("WARN log, current thread id = %d\n", curTid);
        LOG_ERROR("ERROR log, current thread id = %d\n", curTid);
    }
}

std::unique_ptr<SynLog> synLog;

void synOutput(const char *msg, size_t len, size_t keyLen = 0) {
    synLog->append(msg, len);
}

void SynLogger() {
    LogConfig config;
    config.logLevel = LogConfig::TRACE;
    config.logFileOptions.baseName = "test-Synlog";
    config.logFileOptions.fileWriterType = FileWriterType::FileWriter_NORMAL;
    // 单个日志文件大小上限设置为 500 M；
    config.logFileOptions.rollSize = static_cast<size_t>(200 * 1024 * 1024);
    Logger::setConfig(config);
    synLog = std::make_unique<SynLog>();
    Logger::setOutputFunc(synOutput);
}

int main() {
    auto start_time = std::chrono::system_clock::now();

    SynLogger();

    std::vector<std::thread> threads(8);

    for (auto &t : threads)
        t = std::thread(testFunc);

    for (auto &t : threads)
        t.join();

    auto end_time = std::chrono::system_clock::now();
    std::chrono::milliseconds cost_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "SynLogger cost time = " << cost_time.count() << " ms" << std::endl;
    return 0;
}