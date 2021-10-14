#include "db.h"

namespace sylar {

#define XX(type, fun) \
type ISQLData::fun(const std::string& name) { \
    int idx = getColumnIndex(name); \
    if(idx == -1) { \
        throw std::logic_error("column [" + name + "] not exists"); \
    } \
    return fun(idx); \
}

    XX(bool, isNull);
    XX(int8_t, getInt8);
    XX(uint8_t, getUint8);
    XX(int16_t, getInt16);
    XX(uint16_t, getUint16);
    XX(int32_t, getInt32);
    XX(uint32_t, getUint32);
    XX(int64_t, getInt64);
    XX(uint64_t, getUint64);
    XX(float, getFloat);
    XX(double, getDouble);
    XX(std::string, getString);
    XX(std::string, getBlob);
    XX(time_t, getTime);
#undef XX

}
