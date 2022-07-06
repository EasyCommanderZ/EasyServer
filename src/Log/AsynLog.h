#ifndef __SRC_LOG_ASYNLOG_H_
#define __SRC_LOG_ASYNLOG_H_

#include "Log/CountDownLatch.h"
#include "Log/FileWriter.h"
#include "Log/LogBuffer.h"
#include "Log/SynLog.h"
#include "Util/noncopyable.h"
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 异步日志类
class AsynLog : noncopyable {
private:
    void threadFunc();
    using Buffer = LogBuffer::FixedBuffer<LogBuffer::BufferSize>;

    // 日志文件配置
    const std::string _basename;
    const size_t _rollSize;
    const int _flushInterval;

    std::atomic_bool _started;
    std::thread _thread;
    CountDownLatch _counter;

    std::mutex _mtx;
    std::condition_variable _cv;

    // 缓冲区节点，用于实现环形缓冲
    struct BufferNode {
        std::unique_ptr<Buffer> _buff;
        std::shared_ptr<BufferNode> _prev, _next;
        BufferNode() :
            _buff(nullptr), _prev(nullptr), _next(nullptr){};
        BufferNode(Buffer *_data) :
            _buff(_data), _prev(nullptr), _next(nullptr){};
    };

    using BufferNodePtr = std::shared_ptr<BufferNode>;

    // 环形缓冲区头尾节点，哨兵节点，无数据
    BufferNodePtr head, tail;
    int _bufferSize;

    // 操作缓冲区函数
    void addToHead(BufferNodePtr);
    void addToTail(BufferNodePtr);
    BufferNodePtr removeHead();
    BufferNodePtr removeTail();
    BufferNodePtr newBufferNode();

    // 执行当前写入的 BufferNode
    BufferNodePtr curBuffNode;
    // 后台线程要落盘的buffer，已经写满或因刷盘间隔而被 threadFunc 放入 writeBuffer中
    std::vector<BufferNodePtr> writeBufferNode;

    FileWriterType _fileWriterType;

public:
    AsynLog(const std::string &basename = dLogConfig.logFileOptions.baseName,
            size_t rollSize = dLogConfig.logFileOptions.rollSize,
            int flushInterval = dLogConfig.logFileOptions.flushInterval,
            int _bufferNums = dLogConfig.logFileOptions.bufferNums,
            FileWriterType fileWriterType = dLogConfig.logFileOptions.fileWriterType);

    ~AsynLog();

    void append(const char *msg, size_t len, size_t keylen = 0);

    void start() {
        if (_counter.getCount() == 0) return;
        _started = true;
        _thread = std::thread(&AsynLog::threadFunc, this);
        _counter.wait();
    }

    void stop() {
        assert(_started == true);
        _started = false;
        _cv.notify_one();
        _thread.join();
    }
};

#endif /* __SRC_LOG_ASYNLOG_H_ */
