#ifndef __SYLAR_PACK_BYTEARRAY_DECODER_H__
#define __SYLAR_PACK_BYTEARRAY_DECODER_H__

#include "sylar/bytearray.h"
#include "pack.h"
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace sylar {
namespace pack {

class ByteArrayDecoder {
public:
    ByteArrayDecoder(ByteArray::ptr ba)
        :m_value(ba) {
    }

#define XX_DECODE(ctype, type) \
    bool decode(const std::string& name, ctype& v, const PackFlag& flag) { \
        v = m_value->read ## type(); \
        return true; \
    } \
    bool decode(ctype& v, const PackFlag& flag) { \
        v = m_value->read ## type(); \
        return true; \
    }

    XX_DECODE(bool,     Int32);
    XX_DECODE(int8_t,   Int32);
    XX_DECODE(int16_t,  Int32);
    XX_DECODE(int32_t,  Int32);
    XX_DECODE(int64_t,  Int64);
    XX_DECODE(uint8_t,  Uint32);
    XX_DECODE(uint16_t, Uint32);
    XX_DECODE(uint32_t, Uint32);
    XX_DECODE(uint64_t, Uint64);
    XX_DECODE(float,    Float);
    XX_DECODE(double,   Double);
    XX_DECODE(std::string,   StringVint);
#undef XX_DECODE

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
        v.__sylar_decode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(T& v, const PackFlag& flag) {
        v.__sylar_decode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode_inherit(T& v, const PackFlag& flag) {
        v.__sylar_decode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
        __sylar_decode__(*this, v, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(T& v, const PackFlag& flag) {
        __sylar_decode__(*this, v, flag);
        return true;
    }

#define XX_DECODE(arr, fun) \
    template<class T, class... Args> \
    bool decode(const std::string& name, arr<T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto size = m_value->readUint32(); \
        for(uint32_t i = 0; i < size; ++i) { \
            T t; \
            decode(t, flag); \
            v.fun(t); \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool decode(arr<T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto size = m_value->readUint32(); \
        for(uint32_t i = 0; i < size; ++i) { \
            T t; \
            decode(t, flag); \
            v.fun(t); \
        } \
        return true; \
    }

    XX_DECODE(std::vector,             emplace_back);
    XX_DECODE(std::list,               emplace_back);
    XX_DECODE(std::set,                emplace);
    XX_DECODE(std::unordered_set,      emplace);
#undef XX_DECODE

#define XX_DECODE(m) \
    template<class K, class V, class... Args> \
    bool decode(const std::string& name, m<K, V, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto size = m_value->readUint32(); \
        for(uint32_t i = 0; i < size; ++i) { \
            K tk; \
            V tv; \
            decode(tk, flag); \
            decode(tv, flag); \
            v[tk] = tv; \
        } \
        return true; \
    } \
    template<class K, class V, class... Args> \
    bool decode(m<K, V, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto size = m_value->readUint32(); \
        for(uint32_t i = 0; i < size; ++i) { \
            K tk; \
            V tv; \
            decode(tk, flag); \
            decode(tv, flag); \
            v[tk] = tv; \
        } \
        return true; \
    }
    XX_DECODE(std::map);
    XX_DECODE(std::unordered_map);
#undef XX_DECODE

#define XX_DECODE(type, fun) \
    template<class T, class... Args> \
    bool decode(const std::string& name, type<T, Args...>& v, const PackFlag& flag) { \
        v = fun(); \
        return decode(name, *v, flag); \
    } \
    template<class T, class... Args> \
    bool decode(type<T, Args...>& v, const PackFlag& flag) { \
        v = fun(); \
        return decode(*v, flag); \
    }

    XX_DECODE(std::shared_ptr, std::make_shared<T>);
    XX_DECODE(std::unique_ptr, std::make_unique<T>);

#undef XX_DECODE
    ByteArray::ptr getValue() { return m_value;}
private:
    ByteArray::ptr m_value;
};

template<class T>
bool DecodeFromByteArray(ByteArray::ptr ba, T& value, const PackFlag& flag) {
    ByteArrayDecoder jd(ba);
    return jd.decode(value, flag);
}

template<class T>
bool DecodeFromByteArrayString(const std::string& data, T& value, const PackFlag& flag) {
    ByteArray::ptr ba(new ByteArray((void*)data.c_str(), data.size()));
    return DecodeFromByteArray(ba, value, flag);
}

}
}

#endif
