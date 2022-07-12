#include "Http/HttpData.h"
#include "Log/AsynLog.h"
#include "Log/LogConfig.h"
#include "Log/Logger.h"
#include "Reactor/EventLoop.h"
#include "Server/Server.h"
#include <cstdlib>
#include <memory>
#include <string>
#include <getopt.h>


int main(int argc, char *argv[]) {
    int threadNum = 4;
    int port = 1316;
    std::string logName = "EasyServerLog";

    // parse args
    int opt;
    const char *str = "t:l:p:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
        case 't': {
            threadNum = atoi(optarg);
            break;
        }
        case 'l': {
            logName = optarg;
            if (logName.size() < 2 || optarg[0] != '/') {
                printf("logPath should start with \"/\"\n");
                abort();
            }
            if(logName.empty()) {
                printf("Empty LogName");
                abort();
            }
            break;
        }
        case 'p': {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }

    std::unique_ptr<AsynLog> asynLog;
    LogConfig config;
    config.logLevel = LogConfig::INFO;
    config.logFileOptions.baseName = logName;
    config.logFileOptions.fileWriterType = FileWriterType::FileWriter_NORMAL;
    config.logFileOptions.rollSize = static_cast<size_t>(500 * 1024 * 1024);
    Logger::setConfig(config);
    // auto asynOuput = [&](const char *msg, size_t len, size_t keyLen = 0) {
    //     asynLog->append(msg, len);
    // };
    // Logger::setOutputFunc(asynOutput);
    asynLog = std::make_unique<AsynLog>();
    asynLog->start();
    
    EventLoop mainLoop;
    Server EasyServer(&mainLoop, threadNum, port);
    EasyServer.start();
    mainLoop.loop();

    return 0;
}