#ifndef __SRC_UTIL_THREADINFO_H_
#define __SRC_UTIL_THREADINFO_H_
#include "Util/noncopyable.h"
#include <arm/types.h>
#include <cstddef>
#include <cstdio>
#include <sstream>
#include <string>
#include <sys/_types/_pid_t.h>
#include <thread>
#include <unistd.h>

namespace ThreadInfo {
    extern thread_local std::thread::id currentTID;
    extern thread_local std::string tidStr;
    extern thread_local size_t tidStrlen;

    std::thread::id getTid() {
        if(currentTID != std::this_thread::get_id()) {
            currentTID = std::this_thread::get_id();
            std::ostringstream ss;
            ss << currentTID;
            tidStr = ss.str();
            tidStrlen = static_cast<size_t>(tidStr.size());
        }
        return currentTID;
    }

    const std::string getTidStr() {
        return tidStr;
    }

    const size_t getTidNum() {
        size_t ret = std::stoull(tidStr);
        return ret;
    }

    size_t getTidStrlen() {
        return tidStrlen;
    }

    pid_t getPid() {
        return ::getpid();
    }

    std::string getPidStr() {
        return std::to_string(getPid());
    }
}

#endif /* __SRC_UTIL_THREADINFO_H_ */
