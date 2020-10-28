#include "stream.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

int Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    while(left > 0) {
        int64_t len = read((char*)buffer + offset, left);
        if(len <= 0) {
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length;
    while(left > 0) {
        int64_t len = read(ba, left);
        if(len <= 0) {
            return len;
        }
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    //static const int64_t MAX_LEN = 1024 * 16;
    while(left > 0) {
        //int64_t len = write((const char*)buffer + offset, std::min(left, MAX_LEN));
        int64_t len = write((const char*)buffer + offset, left);
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize fail length=" << length
                << " left=" << left << " errno=" << errno << ", " << strerror(errno);
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;

}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length;
    while(left > 0) {
        int64_t len = write(ba, left);
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize fail length=" << length
                << " errno=" << errno << ", " << strerror(errno);
            return len;
        }
        left -= len;
    }
    return length;
}

}
