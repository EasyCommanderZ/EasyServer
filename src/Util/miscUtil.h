#ifndef __SRC_UTIL_MISCUTIL_H_
#define __SRC_UTIL_MISCUTIL_H_

#include <sstream>
#include <string>
#include <thread>
const auto tidToStr(std::thread::id tid){
    std::ostringstream ss;
    ss << tid;
    std::string str = ss.str();
    auto ret = str.c_str();
    return ret;
};



#endif /* __SRC_UTIL_MISCUTIL_H_ */
