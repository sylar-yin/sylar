#ifndef __SYLAR_PACK_RAPIDJSON_ENCODE_H__
#define __SYLAR_PACK_RAPIDJSON_ENCODE_H__

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "pack.h"
#include <string.h>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>

namespace sylar {
namespace pack {

class RapidJsonEncoder {
public:
    RapidJsonEncoder()
        :m_writer(m_buffer) {
    }

    //template<class T>
    //SYLAR_NOT_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
    //    (*m_cur)[name] = v;
    //    return true;
    //}
    //template<class T>
    //SYLAR_NOT_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
    //    (*m_cur) = v;
    //    return true;
    //}

#define XX_ENCODE(ctype, type) \
    bool encode(const std::string& name, const ctype& v, const PackFlag& flag) { \
        m_writer.Key(name.c_str()); \
        m_writer.type(v); \
        return true; \
    } \
    bool encode(const ctype& v, const PackFlag& flag) { \
        m_writer.type(v); \
        return true; \
    }
    XX_ENCODE(bool,     Bool);
    XX_ENCODE(int8_t,   Int);
    XX_ENCODE(int16_t,  Int);
    XX_ENCODE(int32_t,  Int);
    XX_ENCODE(long long,  Int64);
    XX_ENCODE(int64_t,  Int64);
    XX_ENCODE(uint8_t,  Uint);
    XX_ENCODE(uint16_t, Uint);
    XX_ENCODE(uint32_t, Uint);
    XX_ENCODE(unsigned long long, Uint64);
    XX_ENCODE(uint64_t, Uint64);
    XX_ENCODE(double,   Double);
    XX_ENCODE(float,    Double);
#undef XX_ENCODE

    bool encode(const std::string& name, const std::string& v, const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.String(v.c_str(), v.size());
        return true;
    }

    bool encode(const std::string& v, const PackFlag& flag) {
        m_writer.String(v.c_str(), v.size());
        return true;
    }

    bool encode(const std::string& name, char v, const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.String(&v, 1);
        return true;
    }

    bool encode(char v, const PackFlag& flag) {
        m_writer.String(&v, 1);
        return true;
    }


    template<class T, int N>
    bool encode(const std::string& name, const T (&v)[N], const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.StartArray();
        for(size_t i = 0; i < N; ++i) {
            encode(v[i], flag);
        }
        m_writer.EndArray();
        return true;
    }

    template<class T, int N>
    bool encode(const T (&v)[N], const PackFlag& flag) {
        m_writer.StartArray();
        for(size_t i = 0; i < N; ++i) {
            encode(v[i], flag);
        }
        m_writer.EndArray();
        return true;
    }

    bool encode(const std::string& name, const char* v, const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.String(v, strlen(v));
        return true;
    }
    bool encode(const char* v, const PackFlag& flag) {
        m_writer.String(v, strlen(v));
        return true;
    }


    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.StartObject();
        v.__sylar_encode__(*this, flag);
        m_writer.EndObject();
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
        m_writer.StartObject();
        v.__sylar_encode__(*this, flag);
        m_writer.EndObject();
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T* v, const PackFlag& flag) {
        if(v) {
            return encode(name, *v, flag);
        } else {
            m_writer.Key(name.c_str());
            m_writer.Null();
            return true;
        }
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const T* v, const PackFlag& flag) {
        if(v) {
            return encode(*v, flag);
        } else {
            m_writer.Null();
            return true;
        }
    }


    template<class T>
    SYLAR_IS_PACK(T, bool) encode_inherit(const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        m_writer.Key(name.c_str());
        m_writer.StartObject();
        __sylar_encode__(*this, v, flag);
        m_writer.EndObject();
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const T& v, const PackFlag& flag) {
        m_writer.StartObject();
        __sylar_encode__(*this, v, flag);
        m_writer.EndObject();
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const std::string& name, const T* v, const PackFlag& flag) {
        if(v) {
            return encode(name, *v, flag);
        } else {
            m_writer.Key(name.c_str());
            m_writer.Null();
            return true;
        }
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const T* v, const PackFlag& flag) {
        if(v) {
            return encode(*v, flag);
        } else {
            m_writer.Null();
            return true;
        }
    }


#define XX_ENCODE(arr) \
    template<class T, class... Args> \
    bool encode(const std::string& name, const arr<T, Args...>& v, const PackFlag& flag) { \
        m_writer.Key(name.c_str()); \
        m_writer.StartArray(); \
        for(auto& i : v) { \
            encode(i, flag); \
        } \
        m_writer.EndArray(); \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const arr<T, Args...>& v, const PackFlag& flag) { \
        m_writer.StartArray(); \
        for(auto& i : v) { \
            encode(i, flag); \
        } \
        m_writer.EndArray(); \
        return true; \
    }
    XX_ENCODE(std::vector);
    XX_ENCODE(std::list);
    XX_ENCODE(std::set);
    XX_ENCODE(std::unordered_set);
#undef XX_ENCODE

#define XX_ENCODE(m) \
    template<class T, class... Args> \
    bool encode(const std::string& name, const m<std::string, T, Args...>& v, const PackFlag& flag) { \
        m_writer.Key(name.c_str()); \
        m_writer.StartObject(); \
        for(auto& i : v) { \
            encode(i.first, i.second, flag); \
        } \
        m_writer.EndObject(); \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const m<std::string, T, Args...>& v, const PackFlag& flag) { \
        m_writer.StartObject(); \
        for(auto& i : v) { \
            encode(i.first, i.second, flag); \
        } \
        m_writer.EndObject(); \
        return true; \
    }
    XX_ENCODE(std::map);
    XX_ENCODE(std::unordered_map);
#undef XX_ENCODE

#define XX_ENCODE(type, ...) \
    template<class T> \
    bool encode(const std::string& name, const type<T>& v, const PackFlag& flag) { \
        if(v __VA_ARGS__) { \
            return encode(name, *v, flag); \
        } else { \
            /*//TODO null*/ \
        } \
        return true; \
    } \
    template<class T> \
    bool encode(const type<T>& v, const PackFlag& flag) { \
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

    std::string getValue() { return m_buffer.GetString();}
private:
    rapidjson::StringBuffer m_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> m_writer;
    bool m_hasKey = false;
};

template<class T>
std::string EncodeToRapidJsonString(const T& v, const PackFlag& flag) {
    RapidJsonEncoder je;
    je.encode(v, flag);
    return je.getValue();
}

}
}

#endif
