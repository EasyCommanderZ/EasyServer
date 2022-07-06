#ifndef __SRC_LOG_BUFFER_H_
#define __SRC_LOG_BUFFER_H_

#include "Util/noncopyable.h"
#include <cstddef>
#include <cstring>

namespace LogBuffer {
// 使用命名空间，可以实现不同种类的缓存类型

// 日志记录行缓冲区大小 1Kb
const int lineBufferSize = 1024;

// 异步日单个缓冲区大小：4Mb
const int BufferSize = 4 * 1024 * 1024;

template <size_t SIZE>
class FixedBuffer : noncopyable {
private:
    char _data[SIZE]; // 缓冲区
    size_t _cur;      // 当前已经使用的大小
public:
    FixedBuffer() :
        _cur(0){};

    ~FixedBuffer() = default;

    size_t size() const {
        return _cur;
    };

    void clear() {
        _cur = 0;
        // memset(_data, 0, sizeof _data);
        // or : bzero();
        // 缺少这一步应该不影响，在后续的操作中会直接从 _cur 开始覆盖，读取也是根据 _cur 来进行读取的
    }

    size_t avail() const {
        return SIZE - _cur;
    }

    const char* data() const {
        return _data;
    }

    char* current() {
        return _data + _cur;
    }

    void addLen(size_t len) {
        _cur += len;
    }

    void bzero() {
        memset(_data, 0, sizeof _data);
    }

    void append(const char* msg, size_t len) {
        if(avail() > len) {
            memcpy(_data + _cur, msg, len);
            _cur += len;
        }
    }


};
}; // namespace LogBuffer

#endif /* __SRC_LOG_BUFFER_H_ */
