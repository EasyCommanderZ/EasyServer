#ifndef __SRC_UTIL_MISCUTIL_H_
#define __SRC_UTIL_MISCUTIL_H_

#include <sstream>
#include <string>
#include <thread>
#include <sys/syscall.h>
#include <unistd.h>

auto tidToStr(std::thread::id tid) {
    std::ostringstream ss;
    ss << tid;
    std::string str = ss.str();
    return str;
};

#endif /* __SRC_UTIL_MISCUTIL_H_ */
