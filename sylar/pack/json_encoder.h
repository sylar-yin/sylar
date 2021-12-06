#ifndef __SYLAR_PACK_JSON_ENCODE_H__
#define __SYLAR_PACK_JSON_ENCODE_H__

#include <json/json.h>
#include "pack.h"
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

namespace sylar {
namespace pack {

class JsonEncoder {
public:
    JsonEncoder()
        :m_cur(&m_value) {
    }

    template<class T>
    SYLAR_NOT_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        (*m_cur)[name] = v;
        return true;
    }

    template<class T>
    SYLAR_NOT_PACK(T, bool) encode(const T& v, const PackFlag& flag) {
        (*m_cur) = v;
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) encode(const std::string& name, const T& v, const PackFlag& flag) {
        auto cur = m_cur;
        m_cur = &(*m_cur)[name];
        v.__sylar_encode__(*this, flag);
        m_cur = cur;
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
        m_cur = &(*m_cur)[name];
        __sylar_encode__(*this, v, flag);
        m_cur = cur;
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
        auto& mm = (*cur)[name]; \
        for(auto& i : v) { \
            Json::Value t; \
            m_cur = &t; \
            encode(i, flag); \
            mm.append(t); \
        } \
        m_cur = cur; \
        return true; \
    } \
    template<class T> \
    bool encode(const arr<T>& v, const PackFlag& flag) { \
        auto cur = m_cur; \
        for(auto& i : v) { \
            Json::Value t; \
            m_cur = &t; \
            encode(i, flag); \
            (*cur).append(t); \
        } \
        m_cur = cur; \
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
        auto& mm = (*cur)[name]; \
        for(auto it = v.begin(); it != v.end(); ++it) { \
            m_cur = &mm[it->first]; \
            encode(it->second, flag); \
        } \
        m_cur = cur; \
        return true; \
    } \
    template<class T> \
    bool encode(const m<std::string, T>& v, const PackFlag& flag) { \
        auto cur = m_cur; \
        for(auto it = v.begin(); it != v.end(); ++it) { \
            Json::Value t; \
            m_cur = &(*cur)[it->first]; \
            encode(it->second, flag); \
        } \
        m_cur = cur; \
        return true; \
    }
    XX_ENCODE(std::map);
    XX_ENCODE(std::unordered_map);
#undef XX_ENCODE

    Json::Value& getValue() { return m_value;}
private:
    Json::Value m_value;
    Json::Value* m_cur;
};

template<class T>
Json::Value EncodeToJson(const T& v, const PackFlag& flag) {
    JsonEncoder je;
    je.encode(v, flag);
    return je.getValue();
}

template<class T>
std::string EncodeToJsonString(const T& v, const PackFlag& flag) {
    auto value = EncodeToJson(v, flag);
    return sylar::JsonUtil::ToString(value);
}

}
}

#endif
