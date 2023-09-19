#ifndef __SYLAR_RAPIDPACK_JSON_DECODER_H__
#define __SYLAR_RAPIDPACK_JSON_DECODER_H__

#include <rapidjson/document.h>
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

class RapidJsonDecoder {
public:
    RapidJsonDecoder(const rapidjson::Document& doc)
        :m_value(&doc)
        ,m_cur(m_value) {
    }

    //template<class T>
    //SYLAR_NOT_PACK(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
    //    if((*m_cur)[name].isNull()) {
    //        return true;
    //    }
    //    v = boost::lexical_cast<T>((*m_cur)[name].asString());
    //    return true;
    //}

#define XX_DECODE(ctype, type) \
    bool decode(const std::string& name, ctype& v, const PackFlag& flag) { \
        auto tmp = get(name.c_str()); \
        if(tmp == nullptr) { \
            return true; \
        } else if(tmp->Is##type()) { \
            v = tmp->Get##type(); \
            return true; \
        } else if(tmp->IsString()) {\
            v = boost::lexical_cast<ctype>(tmp->GetString()); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    } \
    bool decode(ctype& v, const PackFlag& flag) { \
        if(m_cur->Is##type()) { \
            v = m_cur->Get##type(); \
        } else if(m_cur->IsString()) { \
            v = boost::lexical_cast<ctype>(m_cur->GetString()); \
        } else { \
            /*//TODO*/ \
        } \
        return true; \
    }

    XX_DECODE(int8_t,   Int);
    XX_DECODE(int16_t,  Int);
    XX_DECODE(int32_t,  Int);
    XX_DECODE(long long,  Int64);
    XX_DECODE(int64_t,  Int64);
    XX_DECODE(uint8_t,  Uint);
    XX_DECODE(uint16_t, Uint);
    XX_DECODE(uint32_t, Uint);
    XX_DECODE(unsigned long long, Uint64);
    XX_DECODE(uint64_t, Uint64);
    XX_DECODE(float,    Double);
    XX_DECODE(double,   Double);
#undef XX_DECODE
    bool decode(const std::string& name, bool& v, const PackFlag& flag) {
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            v = false;
            return true;
        }
        if(tmp->IsBool()) {
            v = tmp->GetBool();
        } else if(tmp->IsInt64()) {
            v = tmp->GetInt64() != 0;
        }
        return true;
    }

    bool decode(const std::string& name, char& v, const PackFlag& flag) {
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            v = false;
            return true;
        }
        v = tmp->GetString()[0];
        return true;
    }


    bool decode(const std::string& name, std::string& v, const PackFlag& flag) {
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            return true;
        }
        v = tmp->GetString();
        return true;
    }

    bool decode(const std::string& name, char* v, const PackFlag& flag) {
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            return true;
        }
        strncpy(v, tmp->GetString(), tmp->GetStringLength());
        return true;
    }

    bool decode(bool& v, const PackFlag& flag) {
        if(m_cur->IsBool()) {
            v = m_cur->GetBool();
        } else if(m_cur->IsInt64()) {
            v = m_cur->GetInt64() != 0;
        }
        return true;
    }

    bool decode(char& v, const PackFlag& flag) {
        v = m_cur->GetString()[0];
        return true;
    }


    bool decode(std::string& v, const PackFlag& flag) {
        v = m_cur->GetString();
        return true;
    }

    bool decode(char* v, const PackFlag& flag) {
        strncpy(v, m_cur->GetString(), m_cur->GetStringLength());
        return true;
    }

    template<class T, int N>
    bool decode(const std::string& name, T(&v)[N], const PackFlag& flag) {
        memset(v, 0, sizeof(T) * N);
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            return true;
        }
        if(tmp->IsArray()) {
            auto cur = m_cur;
            int idx = 0;
            for(auto it = tmp->Begin(); it != tmp->End(); ++it) {
                m_cur = &*it;
                decode(v[idx], flag);
                ++idx;
            }
            m_cur = cur;
        } else {
        }
        return true;
    }

    template<class T, int N>
    bool decode(T(&v)[N], const PackFlag& flag) {
        memset(v, 0, sizeof(T) * N);
        if(m_cur->IsArray()) {
            auto cur = m_cur;
            int idx = 0;
            for(auto it = cur->Begin(); it != cur->End(); ++it) {
                m_cur = &*it;
                decode(v[idx], flag);
                ++idx;
            }
            m_cur = cur;
        } else {
            /*//TODO */
        }
        return true;
    }

    template<class T>
    SYLAR_IS_PACK(T, bool) decode(const std::string& name, T& v, const PackFlag& flag) {
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            return true;
        }
        auto cur = m_cur;
        m_cur = tmp;
        v.__sylar_decode__(*this, flag);
        m_cur = cur;
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
        auto tmp = get(name.c_str());
        if(tmp == nullptr) {
            return true;
        }
        auto cur = m_cur;
        m_cur = tmp;
        __sylar_decode__(*this, v, flag);
        m_cur = cur;
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
        auto tmp = get(name.c_str()); \
        if(tmp == nullptr) { \
            return true; \
        } \
        if(tmp->IsArray()) { \
            auto cur = m_cur; \
            for(auto it = tmp->Begin(); it != tmp->End(); ++it) { \
                m_cur = &*it; \
                T t; \
                decode(t, flag); \
                v.fun(t); \
            } \
            m_cur = cur; \
        } else { \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool decode(arr<T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        if(m_cur->IsArray()) { \
            auto cur = m_cur; \
            for(auto it = cur->Begin(); it != cur->End(); ++it) { \
                m_cur = &*it; \
                T t; \
                decode(t, flag); \
                v.fun(t); \
            } \
            m_cur = cur; \
        } else { \
            /*//TODO */\
        } \
        return true; \
    }

    XX_DECODE(std::vector,             emplace_back);
    XX_DECODE(std::list,               emplace_back);
    XX_DECODE(std::set,                emplace);
    XX_DECODE(std::unordered_set,      emplace);
#undef XX_DECODE

#define XX_DECODE(m) \
    template<class T, class... Args> \
    bool decode(const std::string& name, m<std::string, T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        auto tmp = get(name.c_str()); \
        if(tmp == nullptr) { \
            return true; \
        } \
        if(tmp->IsObject()) { \
            auto cur = m_cur; \
            for(auto it = tmp->MemberBegin(); it != tmp->MemberEnd(); ++it) { \
                m_cur = &(it->value); \
                T t; \
                decode(t, flag); \
                v[it->name.GetString()] = t; \
            } \
            m_cur = cur; \
        } else { \
        } \
        return true; \
    } \
    template<class T, class... Args> \
    bool decode(m<std::string, T, Args...>& v, const PackFlag& flag) { \
        v.clear(); \
        if(m_cur->IsObject()) { \
            auto cur = m_cur; \
            for(auto it = cur->MemberBegin(); it != cur->MemberEnd(); ++it) { \
                m_cur = &(it->value); \
                T t; \
                decode(t, flag); \
                v[it->name.GetString()] = t; \
            } \
            m_cur = cur; \
        } else { \
            /*//TODO */\
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
    //XX_DECODE(std::unique_ptr, std::make_unique<T>);
    XX_DECODE(std::unique_ptr, std::unique_ptr<T>(new T));

#undef XX_DECODE
    const rapidjson::Value* getValue() { return m_value;}
private:
    const rapidjson::Value* get(const char* key) {
        if(key == nullptr) {
            return m_cur;
        } else {
            auto it = m_cur->FindMember(key);
            if(it != m_cur->MemberEnd() && !(it->value.IsNull())) {
                return &it->value;
            }
            return nullptr;
        }
    }
private:
    const rapidjson::Value* m_value;
    const rapidjson::Value* m_cur;
};

template<class T>
bool DecodeFromRapidJson(const rapidjson::Document& json, T& value, const PackFlag& flag) {
    RapidJsonDecoder jd(json);
    return jd.decode(value, flag);
}

template<class T>
bool DecodeFromRapidJsonString(const std::string& json, T& value, const PackFlag& flag) {
    rapidjson::Document doc;
    doc.Parse(json.c_str(), json.size());
    return DecodeFromRapidJson(doc, value, flag);
}

}
}

#endif
