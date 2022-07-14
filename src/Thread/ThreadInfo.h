#ifndef __SRC_THREAD_THREADINFO_H_
#define __SRC_THREAD_THREADINFO_H_

#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <sys/syscall.h>


namespace ThreadInfo {
extern thread_local int t_cachedTid;
extern thread_local char t_tidString[32];
extern thread_local int t_tidStringLength;
extern thread_local const char *t_threadName;

void cacheTid();

inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
        cacheTid();
    }
    return t_cachedTid;
}

inline const char *tidString() // for logging
{
    return t_tidString;
}

inline int tidStringLength() // for logging
{
    return t_tidStringLength;
}

inline const char *name() {
    return t_threadName;
}
} // namespace ThreadInfo



#endif /* __SRC_THREAD_THREADINFO_H_ */
