#ifndef __SYLAR_PACK_YAML_DECODER_H__
#define __SYLAR_PACK_YAML_DECODER_H__

#include <yaml-cpp/yaml.h>
#include "pack.h"
#include <vector>
#include <string.h>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace sylar {
namespace pack {

class YamlDecoder {
public:
    YamlDecoder(const YAML::Node& value)
        :m_value(value) {
        m_cur = m_value;
    }

    //template<class T>
    //SYLAR_NOT_PACK(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
    //    if((*m_cur)[name].isNull()) {
    //        return true;
    //    }
    //    v = boost::lexical_cast<T>((*m_cur)[name].asString());
    //    return true;
    //}
#define XX_DECODE(ctype) \
    bool decode(const std::string& name, ctype& v, const PackFlag& flag) { \
        auto n = m_cur[name]; \
        if(n.IsNull()) { \
            return true; \
        } \
        if(n.IsScalar()) { \
            v = n.as<ctype>(); \
        } \
        return true; \
    } \
    bool decode(ctype& v, const PackFlag& flag) { \
        if(m_cur.IsNull()) { \
            return true; \
        } \
        if(m_cur.IsScalar()) { \
            v = m_cur.as<ctype>(); \
        } \
        return true; \
    }


    XX_DECODE(int8_t);
    XX_DECODE(int16_t);
    XX_DECODE(int32_t);
    XX_DECODE(long long);
    XX_DECODE(int64_t);
    XX_DECODE(uint8_t);
    XX_DECODE(uint16_t);
    XX_DECODE(uint32_t);
    XX_DECODE(unsigned long long);
    XX_DECODE(uint64_t);
    XX_DECODE(float);
    XX_DECODE(double);
    XX_DECODE(bool);
#undef XX_DECODE
    bool decode(const std::string& name, std::string& v, const PackFlag& flag) {
        auto n = m_cur[name];
        if(n.IsNull()) {
            return true;
        }
        if(n.IsScalar()) {
            v = n.Scalar();
        }
        return true;
    }

    bool decode(std::string& v, const PackFlag& flag) {
        if(m_cur.IsNull()) {
            return true;
        }
        if(m_cur.IsScalar()) {
            v = m_cur.Scalar();
        }
        return true;
    }

    bool decode(const std::string& name, char* v, const PackFlag& flag) {
        auto n = m_cur[name];
        if(n.IsNull()) {
            return true;
        }
        if(n.IsScalar()) {
            auto t = n.Scalar();
            strncpy(v, t.data(), t.size());
        }
        return true;
    }

    bool decode(char* v, const PackFlag& flag) {
        if(m_cur.IsNull()) {
            return true;
        }
        if(m_cur.IsScalar()) {
            auto t = m_cur.Scalar();
            strncpy(v, t.data(), t.size());
        }
        return true;
    }

    template<class T, int N>
    bool decode(const std::string& name, T (&v)[N], const PackFlag& flag) {
        memset(v, 0, sizeof(T) * N);
        auto n = m_cur[name];
        if(n.IsNull()) {
            return true;
        }
        if(n.IsSequence()) {
            auto cur = m_cur;
            int idx = 0;
            for(auto it = n.begin(); it != n.end(); ++it) {
                m_cur.reset(*it);
                decode(v[idx], flag);
                ++idx;
            }
            m_cur.reset(cur);
        } else {
            /*//TODO*/
        }
        return true;
    }
    template<class T, int N>
    bool decode(T (&v)[N], const PackFlag& flag) {
        memset(v, 0, sizeof(T) * N);
        if(m_cur.IsSequence()) {
            auto cur = m_cur;
            int idx = 0;
            for(auto it = cur.begin(); it != cur.end(); ++it) {
                m_cur.reset(*it);
                decode(v[idx], flag);
                ++idx;
            }
            m_cur.reset(cur);
        } else {
            /*//TODO*/
        }
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
        auto n = m_cur[name];
        if(n.IsNull()) {
            return true;
        }
        auto cur = m_cur;
        m_cur.reset(n);
        v.__sylar_decode__(*this, flag);
        m_cur.reset(cur);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(T& v, const PackFlag& flag) {
        v.__sylar_decode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(const std::string& name, T* v, const PackFlag& flag) {
        return decode(name, *v, flag);
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(T* v, const PackFlag& flag) {
        return decode(*v, flag);
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode_inherit(T& v, const PackFlag& flag) {
        v.__sylar_decode__(*this, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
        auto n = m_cur[name];
        if(n.IsNull()) {
            return true;
        }
        auto cur = m_cur;
        m_cur.reset(n);
        __sylar_decode__(*this, v, flag);
        m_cur.reset(cur);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(T& v, const PackFlag& flag) {
        __sylar_decode__(*this, v, flag);
        return true;
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(const std::string& name, T* v, const PackFlag& flag) {
        return decode(name, *v, flag);
    }

    template<class T>
    SYLAR_IS_PACK_OUT(T, bool) decode(T* v, const PackFlag& flag) {
        return decode(*v, flag);
    }


#define XX_DECODE(arr, fun) \
    template<class T, class... Args> \
    bool decode(const std::string& name, arr<T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto n = m_cur[name]; \
        if(n.IsNull()) { \
            return true; \
        } \
        if(n.IsSequence()) { \
            auto cur = m_cur; \
            for(auto it = n.begin(); it != n.end(); ++it) { \
                m_cur.reset(*it); \
                T t; \
                decode(t, flag); \
                v.fun(t); \
            } \
            m_cur.reset(cur); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool decode(arr<T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        if(m_cur.IsSequence()) { \
            auto cur = m_cur; \
            for(auto it = cur.begin(); it != cur.end(); ++it) { \
                m_cur.reset(*it); \
                T t; \
                decode(t, flag); \
                v.fun(t); \
            } \
            m_cur.reset(cur); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    }
    XX_DECODE(std::vector,          emplace_back);
    XX_DECODE(std::list,            emplace_back);
    XX_DECODE(std::set,             emplace);
    XX_DECODE(std::unordered_set,   emplace);
#undef XX_DECODE

#define XX_DECODE(m) \
    template<class T, class... Args> \
    bool decode(const std::string& name, m<std::string, T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto n = m_cur[name]; \
        if(n.IsNull()) { \
            return true; \
        } \
        if(n.IsMap()) { \
            auto cur = m_cur; \
            for(auto it = n.begin(); it != n.end(); ++it) { \
                m_cur.reset(it->second); \
                T t; \
                decode(t, flag); \
                v[it->first.Scalar()] = t; \
            } \
            m_cur.reset(cur); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool decode(m<std::string, T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        if(m_cur.IsMap()) { \
            auto cur = m_cur; \
            for(auto it = cur.begin(); it != cur.end(); ++it) { \
                m_cur.reset(it->second); \
                T t; \
                decode(t, flag); \
                v[it->first.Scalar()] = t; \
            } \
            m_cur.reset(cur); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    }
    XX_DECODE(std::map);
    XX_DECODE(std::unordered_map);
#undef XX_DECODE

#define XX_DECODE(type, fun) \
    template<class T> \
    bool decode(const std::string& name, type<T>& v, const PackFlag& flag) { \
        v = fun(); \
        return decode(name, *v, flag); \
    } \
    template<class T> \
    bool decode(type<T>& v, const PackFlag& flag) { \
        v = fun(); \
        return decode(*v, flag); \
    }

    XX_DECODE(std::shared_ptr, std::make_shared<T>);
    //XX_DECODE(std::unique_ptr, std::make_unique<T>);
    XX_DECODE(std::unique_ptr, std::unique_ptr<T>(new T));
#undef XX_DECODE

    YAML::Node& getValue() { return m_value;}
private:
    YAML::Node m_value;
    YAML::Node m_cur;
};

template<class T>
bool DecodeFromYaml(const YAML::Node& yaml, T& value, const PackFlag& flag) {
    YamlDecoder yd(yaml);
    return yd.decode(value, flag);
}

template<class T>
bool DecodeFromYamlString(const std::string& yaml, T& value, const PackFlag& flag) {
    YAML::Node yvalue = YAML::Load(yaml);
    return DecodeFromYaml(yvalue, value, flag);
}

}
}

#endif
