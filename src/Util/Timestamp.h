#ifndef __SRC_UTIL_TIMESTAMP_H_
#define __SRC_UTIL_TIMESTAMP_H_
#include <cstdint>
#include <string>

class Timestamp {
private:
    int64_t _milliSecondsSinceEpoch{}; // 毫秒级精度

public:
    Timestamp() = default;
    explicit Timestamp(int64_t millSecondsSinceEpoch) :
        _milliSecondsSinceEpoch(millSecondsSinceEpoch){};

    // unix 时间戳
    static Timestamp now();

    // 格式化时间戳
    std::string toString();

    // 获取毫秒
    int64_t getMilliSeconds() {
        return _milliSecondsSinceEpoch;
    };

    // 获取秒
    time_t getSeconds() {
        return static_cast<time_t>(_milliSecondsSinceEpoch / msPerSecond);
    }

    static constexpr int msPerSecond = 1000;
};

#endif /* __SRC_UTIL_TIMESTAMP_H_ */
