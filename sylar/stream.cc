#include "stream.h"
#include "sylar/log.h"
#include "sylar/config.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<int32_t>::ptr g_socket_buff_size =
    sylar::Config::Lookup("socket.buff_size"
            , (int32_t)(1024 * 16)
            , "socket buff size");

int Stream::readFixSize(void* buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    static const int64_t MAX_LEN = g_socket_buff_size->getValue();
    while(left > 0) {
        int64_t len = read((char*)buffer + offset, std::min(left, MAX_LEN));
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "readFixSize fail length=" << length
                << " len=" << len << " errno=" << errno << " errstr=" << strerror(errno);
            return len;
        }
        offset += len;
        left -= len;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    int64_t left = length;
    static const int64_t MAX_LEN = g_socket_buff_size->getValue();
    while(left > 0) {
        int64_t len = read(ba, std::min(left, MAX_LEN));
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "readFixSize fail length=" << length
                << " len=" << len << " errno=" << errno << " errstr=" << strerror(errno);
            return len;
        }
        left -= len;
    }
    return length;
}

int Stream::writeFixSize(const void* buffer, size_t length) {
    size_t offset = 0;
    int64_t left = length;
    static const int64_t MAX_LEN = g_socket_buff_size->getValue();
    while(left > 0) {
        int64_t len = write((const char*)buffer + offset, std::min(left, MAX_LEN));
        //int64_t len = write((const char*)buffer + offset, left);
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize fail length=" << length << " len=" << len
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
        static const int64_t MAX_LEN = g_socket_buff_size->getValue();
        int64_t len = write(ba, std::min(left, MAX_LEN));
        if(len <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "writeFixSize fail length=" << length << " len=" << len
                << " errno=" << errno << ", " << strerror(errno);
            return len;
        }
        left -= len;
    }
    return length;
}

}
