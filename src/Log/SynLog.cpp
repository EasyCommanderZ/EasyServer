#include "Log/SynLog.h"
#include "Util/Timestamp.h"
#include <mutex>

void SynLog::append(const char *msg, size_t len, size_t keyLen) {
    // 同步日志，使用锁
    std::unique_lock lck(_mtx);
    fileWriter.append(msg, len);
    time_t now = Timestamp::now().getSeconds();
    if(static_cast<int>(now - prevSecond) >= _flushInterval) {
        fileWriter.flush();
        prevSecond = now;
    }
}