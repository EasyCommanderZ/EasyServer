#ifndef __SRC_LOG_FILEWRITER_H_
#define __SRC_LOG_FILEWRITER_H_
#include "Util/noncopyable.h"
#include <cstddef>
#include <string>

// 文件读写的抽象类
class FileWriter : noncopyable {
    public:
    FileWriter() = default;
    virtual ~FileWriter() = default;
    // 纯虚函数
    virtual void append(const char* msg, size_t len) = 0;
    virtual void flush() = 0;
    virtual size_t writtenBytes() const = 0;
};

// 当前实现的 FileWriterType
enum FileWriterType {
    NoramalFileWriter = 0
};

class NormalFileWriter : public FileWriter {
    public:
    explicit NormalFileWriter(const std::string &filename) ;
    ~NormalFileWriter() override;

    void append(const char *msg, size_t len) override;
    void flush() override;
    size_t writtenBytes() const override;

    private:
    FILE *_fp;
    // 缓冲区设置为 8KB
    char _buffer[8 * 8 * 1024];
    size_t _written = 0;
};

#endif /* __SRC_LOG_FILEWRITER_H_ */
