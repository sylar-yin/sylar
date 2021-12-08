#ifndef __SYLAR_PACK_BYTEARRAY_ENCODE_H__
#define __SYLAR_PACK_BYTEARRAY_ENCODE_H__

#include "sylar/bytearray.h"
#include "pack.h"
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace sylar {
namespace pack {

class ByteArrayEncoder {
public:
    ByteArrayEncoder() {
        m_value = std::make_shared<ByteArray>();
    }

#define XX_ENCODE(ctype, type) \
    bool encode(const std::string& name, const ctype& v, const PackFlag& flag) { \
        m_value->write ## type(v); \
        return true; \
    } \
    bool encode(const ctype& v, const PackFlag& flag) { \
        m_value->write ## type(v); \
        return true; \
    }
    XX_ENCODE(bool,     Int32);
    XX_ENCODE(int8_t,   Int32);
    XX_ENCODE(int16_t,  Int32);
    XX_ENCODE(int32_t,  Int32);
    XX_ENCODE(int64_t,  Int64);
    XX_ENCODE(uint8_t,  Uint32);
    XX_ENCODE(uint16_t, Uint32);
    XX_ENCODE(uint32_t, Uint32);
    XX_ENCODE(uint64_t, Uint64);
    XX_ENCODE(double,   Double);
    XX_ENCODE(float,    Float);
    XX_ENCODE(std::string,    StringVint);
#undef XX_ENCODE

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode_inherit(const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        __sylar_encode__(*this, v, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const T& v, const PackFlag& flag) {
        __sylar_encode__(*this, v, flag);
        return true;
    }

#define XX_ENCODE(arr) \
    template<class T, class... Args> \
    bool encode(const std::string& name, const arr<T, Args...>& v, const PackFlag& flag) { \
        m_value->writeUint32(v.size()); \
        for(auto& i : v) { \
            encode(i, flag); \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const arr<T, Args...>& v, const PackFlag& flag) { \
        m_value->writeUint32(v.size()); \
        for(auto& i : v) { \
            encode(i, flag); \
        } \
        return true; \
    }
    XX_ENCODE(std::vector);
    XX_ENCODE(std::list);
    XX_ENCODE(std::set);
    XX_ENCODE(std::unordered_set);
#undef XX_ENCODE

#define XX_ENCODE(m) \
    template<class K, class V, class... Args> \
    bool encode(const std::string& name, const m<K, V, Args...>& v, const PackFlag& flag) { \
        m_value->writeUint32(v.size()); \
        for(auto& i : v) { \
            encode(i.first, flag); \
            encode(i.second, flag); \
        } \
        return true; \
    } \
    template<class K, class V, class... Args> \
    bool encode(const m<K, V, Args...>& v, const PackFlag& flag) { \
        m_value->writeUint32(v.size()); \
        for(auto& i : v) { \
            encode(i.first, flag); \
            encode(i.second, flag); \
        } \
        return true; \
    }
    XX_ENCODE(std::map);
    XX_ENCODE(std::unordered_map);
#undef XX_ENCODE

#define XX_ENCODE(type, ...) \
    template<class T, class... Args> \
    bool encode(const std::string& name, const type<T, Args...>& v, const PackFlag& flag) { \
        if(v __VA_ARGS__) { \
            return encode(name, *v, flag); \
        } else { \
            /*//TODO null*/ \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const type<T, Args...>& v, const PackFlag& flag) { \
        if(v __VA_ARGS__) { \
            return encode(*v, flag); \
        } else { \
            /*//TODO null*/ \
        } \
        return true; \
    }

    XX_ENCODE(std::shared_ptr);
    XX_ENCODE(std::unique_ptr);
    XX_ENCODE(std::weak_ptr, && v.lock());
#undef XX_ENCODE

    ByteArray::ptr getValue() { return m_value;}
private:
    ByteArray::ptr m_value;
};

template<class T>
ByteArray::ptr EncodeToByteArray(const T& v, const PackFlag& flag) {
    ByteArrayEncoder je;
    je.encode(v, flag);
    return je.getValue();
}

}
}

#endif
