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

    template<class T>
    SYLAR_NOT_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        m_cur[name] = v;
        return true;
    }

    template<class T>
    SYLAR_NOT_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
        m_cur = v;
        return true;
    }

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

#define XX_ENCODE(arr) \
    template<class T> \
    bool encode(const std::string& name, const arr<T>& v, const PackFlag& flag) { \
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
    template<class T> \
    bool encode(const arr<T>& v, const PackFlag& flag) { \
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
    template<class T> \
    bool encode(const std::string& name, const m<std::string, T>& v, const PackFlag& flag) { \
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
    template<class T> \
    bool encode(const m<std::string, T>& v, const PackFlag& flag) { \
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
