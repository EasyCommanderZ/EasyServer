#include "FileWriter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// 构造函数根据文件名新建位于当前路径的文件
NormalFileWriter::NormalFileWriter(const std::string &filename) {
    // FILE MODE : WE
    _fp = ::fopen(filename.c_str(), "ae");
    if (_fp == nullptr) {
        perror("FileWriter - NormalFileWriter open file failed ");
        int err = ferror(_fp);
        fprintf(stderr, "FileWriter - NormalFileWriter open file : %s failed, errno : %s\n", filename.c_str(), strerror(err));
        abort();
    }
    ::setbuffer(_fp, _buffer, sizeof _buffer);
}

void NormalFileWriter::flush() {
    fflush(_fp);
}

void NormalFileWriter::append(const char *msg, size_t len) {
    size_t written = 0;
    while (written != len) {
        size_t remain = len - written;
        size_t cur = std::fwrite(msg + written, 1, remain, _fp);
        // note : MacOS 上没有 fwrite_unlocked 方法。两者的区别是 _unlocked 版本是线程不安全的，但是效率会更高一些。但是在ZLToolKit中，日志库的写入使用的是fwrite
        // size_t cur = ::fwrite_unlocked(msg + written, 1, remain, _fp);
        if (cur != remain) {
            int err = ferror(_fp);
            if (err) {
                fprintf(stderr, "FileWriter - NormalFileWriter::append failed, errno : %s\n ", strerror(err));
                break;
            }
        }
        written += cur;
    }
    _written += written;
}

size_t NormalFileWriter::writtenBytes() const {
    return _written;
}

// 析构函数中，先刷盘，然后关闭文件描述符
NormalFileWriter::~NormalFileWriter() {
    flush();
    if (_fp != nullptr) {
        ::fclose(_fp);
    }
}