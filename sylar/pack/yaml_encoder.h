#ifndef __SYLAR_PACK_YAML_ENCODER_H__
#define __SYLAR_PACK_YAML_ENCODER_H__

#include <yaml-cpp/yaml.h>
#include "pack.h"
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace sylar {
namespace pack {

class YamlEncoder {
public:
    YamlEncoder() {
        m_cur = m_value;
    }

#define XX_ENCODE(T) \
    bool encode(const std::string& name, const T& v, const PackFlag& flag) { \
        m_cur[name] = v; \
        return true; \
    } \
    bool encode(const T& v, const PackFlag& flag) { \
        m_cur = v; \
        return true; \
    }

    XX_ENCODE(bool);
    XX_ENCODE(int8_t);
    XX_ENCODE(int16_t);
    XX_ENCODE(int32_t);
    XX_ENCODE(int64_t);
    XX_ENCODE(long long);
    XX_ENCODE(uint8_t);
    XX_ENCODE(uint16_t);
    XX_ENCODE(uint32_t);
    XX_ENCODE(unsigned long long);
    XX_ENCODE(uint64_t);
    XX_ENCODE(double);
    XX_ENCODE(float);
    XX_ENCODE(std::string);
#undef XX_ENCODE

    template<class T, int N>
    bool encode(const std::string& name, const T (&v)[N], const PackFlag& flag) {
        auto cur = m_cur;
        auto mm = cur[name];
        for(int i = 0; i < N; ++i) {
            m_cur.reset();
            encode(v[i], flag);
            mm.push_back(m_cur);
        }
        m_cur.reset(cur);
        return true;
    }

    template<class T, int N>
    bool encode(const T (&v)[N], const PackFlag& flag) {
        auto cur = m_cur;
        for(size_t i = 0; i < N; ++i) {
            m_cur.reset();
            encode(v[i], flag);
            cur.push_back(m_cur);
        }
        m_cur.reset(cur);
        return true;
    }

    bool encode(const std::string& name, const char* v, const PackFlag& flag) {
        m_cur[name] = v;
        return true;
    }

    bool encode(const char* v, const PackFlag& flag) {
        m_cur = v;
        return true;
    }


    //template<class T>
    //SYLAR_NOT_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
    //    m_cur[name] = v;
    //    return true;
    //}
    //template<class T>
    //SYLAR_NOT_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
    //    m_cur = v;
    //    return true;
    //}

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        auto cur = m_cur;
        m_cur.reset(m_cur[name]);
        v.__sylar_encode__(*this, flag);
        m_cur.reset(cur);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T* v, const PackFlag& flag) {
        return encode(name, *v, flag);
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const T* v, const PackFlag& flag) {
        return encode(*v, flag);
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode_inherit(const T& v, const PackFlag& flag) {
        v.__sylar_encode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        auto cur = m_cur;
        m_cur.reset(m_cur[name]);
        __sylar_encode__(*this, v, flag);
        m_cur.reset(cur);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const T& v, const PackFlag& flag) {
        __sylar_encode__(*this, v, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const std::string& name, const T* v, const PackFlag& flag) {
        return encode(name, *v, flag);
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) encode(const T* v, const PackFlag& flag) {
        return encode(*v, flag);
    }

#define XX_ENCODE(arr) \
    template<class T, class... Args> \
    bool encode(const std::string& name, const arr<T, Args...>& v, const PackFlag& flag) { \
        auto cur = m_cur; \
        auto mm = cur[name]; \
        for(auto& i : v) { \
            m_cur.reset(); \
            encode(i, flag); \
            mm.push_back(m_cur); \
        } \
        m_cur.reset(cur); \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const arr<T, Args...>& v, const PackFlag& flag) { \
        auto cur = m_cur; \
        for(auto& i : v) { \
            m_cur.reset(); \
            encode(i, flag); \
            cur.push_back(m_cur); \
        } \
        m_cur.reset(cur); \
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
        auto cur = m_cur; \
        auto mm = cur[name]; \
        for(auto& i : v) { \
            m_cur.reset(); \
            encode(i.second, flag); \
            mm[i.first] = m_cur; \
        } \
        m_cur.reset(cur); \
        return true; \
    } \
    template<class T, class... Args> \
    bool encode(const m<std::string, T, Args...>& v, const PackFlag& flag) { \
        auto cur = m_cur; \
        for(auto& i : v) { \
            m_cur.reset(); \
            encode(i.second, flag); \
            cur[i.first] = m_cur; \
        } \
        m_cur.reset(cur); \
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

    const YAML::Node& getValue() const { return m_value;}
    const YAML::Node& getCur() const { return m_cur;}
private:
    YAML::Node m_value;
    YAML::Node m_cur;
};

template<class T>
YAML::Node EncodeToYaml(const T& v, const PackFlag& flag) {
    YamlEncoder ye;
    ye.encode(v, flag);
    return ye.getValue();
}

template<class T>
std::string EncodeToYamlString(const T& v, const PackFlag& flag) {
    auto value = EncodeToYaml(v, flag);
    std::stringstream ss;
    ss << value;
    return ss.str();
}

}
}

#endif
