#include "AsynLog.h"
#include "FileWriter.h"
#include "SynLog.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

AsynLog::AsynLog(const std::string &basename, size_t rollSize, int flushInterval, int _bufferNums, FileWriterType fileWriterType) :
    _basename(basename), _rollSize(rollSize), _flushInterval(flushInterval), _started(false), _counter(1), _bufferSize(_bufferNums), _fileWriterType(fileWriterType), head(std::make_shared<BufferNode>()), tail(std::make_shared<BufferNode>()) {
    head->_next = tail;
    tail->_prev = head;
}

AsynLog::~AsynLog() {
    if (_started) stop();

    // BufferNodePtr 是 shared_ptr
    // shared_ptr 节点会造成循环引用而导致无法释放，所以需要手动解引用
    BufferNodePtr cur = head;
    while (cur != nullptr) {
        BufferNodePtr nxt = cur->_next;
        cur->_prev = cur->_next = nullptr;
        cur = nxt;
    }
}

AsynLog::BufferNodePtr AsynLog::newBufferNode() {
    BufferNodePtr cur = std::make_shared<BufferNode>();
    cur->_buff = std::make_unique<Buffer>();
    return cur;
}

void AsynLog::append(const char *msg, size_t len, size_t keyLen) {
    std::unique_lock<std::mutex> lck(_mtx);
    std::unique_ptr<Buffer> &curBuff = curBuffNode->_buff;

    if (curBuff->avail() > len) {
        curBuff->append(msg, len);
        return;
    }

    // 当缓冲区写满了，从环形缓冲区中移除该bufferNode
    assert(curBuffNode == head->_next);
    writeBufferNode.push_back(curBuffNode);
    removeHead();
    _bufferSize -= 1;

    if (_bufferSize == 0) {
        BufferNodePtr newNode = newBufferNode();
        addToTail(newNode);
        _bufferSize += 1;
    }

    curBuffNode = head->_next;
    curBuffNode->_buff->append(msg, len);

    _cv.notify_one();
}

void AsynLog::addToHead(BufferNodePtr cur) {
    cur->_next = head->_next;
    head->_next->_prev = cur;
    head->_next = cur;
    cur->_prev = head;
}

AsynLog::BufferNodePtr AsynLog::removeHead() {
    assert(_bufferSize != 0);
    BufferNodePtr ret = head->_next;
    head->_next = head->_next->_next;
    head->_next->_prev = head;
    return ret;
}

void AsynLog::addToTail(BufferNodePtr cur) {
    cur->_prev = tail->_prev;
    tail->_prev->_next = cur;
    cur->_next = tail;
    tail->_prev = cur;
}

AsynLog::BufferNodePtr AsynLog::removeTail() {
    assert(_bufferSize != 0);
    BufferNodePtr ret = tail->_prev;
    tail->_prev = tail->_prev->_prev;
    tail->_prev->_next = tail;
    return ret;
}

// 后台日志线程的运行函数
void AsynLog::threadFunc() {
    assert(_started == true);
    assert(_bufferSize > 0);
    _counter.countdown();

    // 构建默认大小的环形缓冲区
    for (int i = 0; i < _bufferSize; i++) {
        auto cur = newBufferNode();
        addToHead(cur);
    }

    // 初始化 curBuffNode
    curBuffNode = head->_next;
    LogFile fileWriter(_basename, _rollSize, _fileWriterType);

    while (_started) {
        // RAII lock block
        {
            std::unique_lock<std::mutex> lck(_mtx);
            // 如果写的太慢会被 flushInterval 间隔唤醒后进行 flush 操作
            // 不能用 while 循环判断缓冲区是否为空，如果缓冲区一直为空，会导致死锁
            if (writeBufferNode.empty()) {
                _cv.wait_for(lck, std::chrono::seconds(_flushInterval));
            }

            // 待写队列是空的 缓冲区也是空的
            if (writeBufferNode.empty() and curBuffNode->_buff->size() == 0) continue;

            // 当前缓冲区有内容 放入待写队列
            writeBufferNode.push_back(curBuffNode);
            removeHead();
            _bufferSize -= 1;

            if (_bufferSize == 0) {
                auto newNode = newBufferNode();
                addToTail(newNode);
                _bufferSize += 1;
            }
            curBuffNode = head->_next;
        }

        // 异步写入日志，限制最大写入缓冲区数目
        int mxWriteNums = std::min(static_cast<int>(writeBufferNode.size()), dLogConfig.logFileOptions.maxBuffToWrite);
        for (int i = 0; i < mxWriteNums; i++) {
            BufferNodePtr node = writeBufferNode[i];
            fileWriter.append(node->_buff->data(), node->_buff->size());
        }

        fileWriter.flush();

        {
            // 归还 bufferNode 到环形缓冲区
            std::unique_lock<std::mutex> lck(_mtx);
            for(auto& node : writeBufferNode) {
                node -> _buff -> clear();
                addToTail(node);
                _bufferSize += 1;
            }
            writeBufferNode.clear();
        }

        fileWriter.flush();
    }
}
