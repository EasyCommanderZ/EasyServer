#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <sys/syscall.h>
#include "ThreadInfo.h"

namespace ThreadInfo {
thread_local int t_cachedTid = 0;
thread_local char t_tidString[32];
thread_local int t_tidStringLength = 6;
thread_local const char *t_threadName = "default";
} // namespace ThreadInfo

pid_t getTid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void ThreadInfo::cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = getTid();
        t_tidStringLength =
            snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}