/**
 * @file util.h
 * @brief 常用的工具函数
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-27
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cxxabi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iomanip>
#include <json/json.h>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <google/protobuf/message.h>
#include "sylar/util/hash_util.h"
#include "sylar/util/json_util.h"
#include "sylar/util/trace.h"
#include "sylar/util/tracker.h"
#include "sylar/util/crypto_util.h"

namespace sylar {

/**
 * @brief 返回当前线程的ID
 */
pid_t GetThreadId();

/**
 * @brief 返回当前协程的ID
 */
uint32_t GetFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

/**
 * @brief 获取当前时间的毫秒
 */
uint64_t GetCurrentMS();

/**
 * @brief 获取当前时间的微秒
 */
uint64_t GetCurrentUS();

std::string ToUpper(const std::string& name);

std::string ToLower(const std::string& name);

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

class FSUtil {
public:
    static void ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix);
    static bool Mkdir(const std::string& dirname);
    static bool IsRunningPidfile(const std::string& pidfile);
    static bool Rm(const std::string& path);
    static bool Mv(const std::string& from, const std::string& to);
    static bool Realpath(const std::string& path, std::string& rpath);
    static bool Symlink(const std::string& frm, const std::string& to);
    static bool Unlink(const std::string& filename, bool exist = false);
    static std::string Dirname(const std::string& filename);
    static std::string Basename(const std::string& filename);
    static bool OpenForRead(std::ifstream& ifs, const std::string& filename
                    ,std::ios_base::openmode mode = std::ios_base::in);
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode = std::ios_base::out);
};

template<class V, class Map, class K>
V GetParamValue(const Map& m, const K& k, const V& def = V()) {
    auto it = m.find(k);
    if(it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<V>(it->second);
    } catch (...) {
    }
    return def;
}

template<class V, class Map, class K>
bool CheckGetParamValue(const Map& m, const K& k, V& v) {
    auto it = m.find(k);
    if(it == m.end()) {
        return false;
    }
    try {
        v = boost::lexical_cast<V>(it->second);
        return true;
    } catch (...) {
    }
    return false;
}

class TypeUtil {
public:
    static int8_t ToChar(const std::string& str);
    static int64_t Atoi(const std::string& str);
    static double Atof(const std::string& str);
    static int8_t ToChar(const char* str);
    static int64_t Atoi(const char* str);
    static double Atof(const char* str);
};

class Atomic {
public:
    template<class T, class S = T>
    static T addFetch(volatile T& t, S v = 1) {
        return __sync_add_and_fetch(&t, (T)v);
    }

    template<class T, class S = T>
    static T subFetch(volatile T& t, S v = 1) {
        return __sync_sub_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T orFetch(volatile T& t, S v) {
        return __sync_or_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T andFetch(volatile T& t, S v) {
        return __sync_and_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T xorFetch(volatile T& t, S v) {
        return __sync_xor_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T nandFetch(volatile T& t, S v) {
        return __sync_nand_and_fetch(&t, (T)v);
    }

    template<class T, class S>
    static T fetchAdd(volatile T& t, S v = 1) {
        return __sync_fetch_and_add(&t, (T)v);
    }

    template<class T, class S>
    static T fetchSub(volatile T& t, S v = 1) {
        return __sync_fetch_and_sub(&t, (T)v);
    }

    template<class T, class S>
    static T fetchOr(volatile T& t, S v) {
        return __sync_fetch_and_or(&t, (T)v);
    }

    template<class T, class S>
    static T fetchAnd(volatile T& t, S v) {
        return __sync_fetch_and_and(&t, (T)v);
    }

    template<class T, class S>
    static T fetchXor(volatile T& t, S v) {
        return __sync_fetch_and_xor(&t, (T)v);
    }

    template<class T, class S>
    static T fetchNand(volatile T& t, S v) {
        return __sync_fetch_and_nand(&t, (T)v);
    }

    template<class T, class S>
    static T compareAndSwap(volatile T& t, S old_val, S new_val) {
        return __sync_val_compare_and_swap(&t, (T)old_val, (T)new_val);
    }

    template<class T, class S>
    static bool compareAndSwapBool(volatile T& t, S old_val, S new_val) {
        return __sync_bool_compare_and_swap(&t, (T)old_val, (T)new_val);
    }
};

template<class T>
void nop(T*) {}

template<class T>
void delete_array(T* v) {
    if(v) {
        delete[] v;
    }
}

template<class T, class ...Args>
inline std::shared_ptr<T> protected_make_shared(Args&&... args) {
    struct Helper : T {
        Helper(Args&&... args)
            :T(std::forward<Args>(args)...) {
        }
    };
    return std::make_shared<Helper>(std::forward<Args>(args)...);
}

template<class T>
class SharedArray {
public:
    explicit SharedArray(const uint64_t& size = 0, T* p = 0)
        :m_size(size)
        ,m_ptr(p, delete_array<T>) {
    }
    template<class D> SharedArray(const uint64_t& size, T* p, D d)
        :m_size(size)
        ,m_ptr(p, d) {
    };

    SharedArray(const SharedArray& r)
        :m_size(r.m_size)
        ,m_ptr(r.m_ptr) {
    }

    SharedArray& operator=(const SharedArray& r) {
        m_size = r.m_size;
        m_ptr = r.m_ptr;
        return *this;
    }

    T& operator[](std::ptrdiff_t i) const {
        return m_ptr.get()[i];
    }

    T* get() const {
        return m_ptr.get();
    }

    bool unique() const {
        return m_ptr.unique();
    }

    long use_count() const {
        return m_ptr.use_count();
    }

    void swap(SharedArray& b) {
        std::swap(m_size, b.m_size);
        m_ptr.swap(b.m_ptr);
    }

    bool operator!() const {
        return !m_ptr;
    }

    operator bool() const {
        return !!m_ptr;
    }

    uint64_t size() const {
        return m_size;
    }
private:
    uint64_t m_size;
    std::shared_ptr<T> m_ptr;
};



class StringUtil {
public:
    static std::string Format(const char* fmt, ...);
    static std::string Formatv(const char* fmt, va_list ap);

    static std::string UrlEncode(const std::string& str, bool space_as_plus = true);
    static std::string UrlDecode(const std::string& str, bool space_as_plus = true);

    static std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");
    static std::string TrimLeft(const std::string& str, const std::string& delimit = " \t\r\n");
    static std::string TrimRight(const std::string& str, const std::string& delimit = " \t\r\n");


    static std::string WStringToString(const std::wstring& ws);
    static std::wstring StringToWString(const std::string& s);

};

std::string GetHostName();
std::string GetIPv4();

bool YamlToJson(const YAML::Node& ynode, Json::Value& jnode);
bool JsonToYaml(const Json::Value& jnode, YAML::Node& ynode);

template<class T>
const char* TypeToName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

std::string PBToJsonString(const google::protobuf::Message& message);

template<class Iter>
std::string Join(Iter begin, Iter end, const std::string& tag) {
    std::stringstream ss;
    for(Iter it = begin; it != end; ++it) {
        if(it != begin) {
            ss << tag;
        }
        ss << *it;
    }
    return ss.str();
}

template<class Iter>
std::string MapJoin(Iter begin, Iter end, const std::string& tag1 = "=", const std::string& tag2 = "&") {
    std::stringstream ss;
    for(Iter it = begin; it != end; ++it) {
        if(it != begin) {
            ss << tag2;
        }
        ss << it->first << "=" << it->second;
    }
    return ss.str();
}

//[begin, end)
//if rt > 0, 存在,返回对应index
//   rt < 0, 不存在,返回对于应该存在的-(index + 1)
template<class T>
int BinarySearch(const T* arr, int length, const T& v) {
    int m = 0;
    int begin = 0;
    int end = length - 1;
    while(begin <= end) {
        m = (begin + end) / 2;
        if(v < arr[m]) {
            end = m - 1;
        } else if(arr[m] < v) {
            begin = m + 1;
        } else {
            return m;
        }
    }
    return -begin - 1;
}

inline bool ReadFixFromStream(std::istream& is, char* data, const uint64_t& size) {
    uint64_t pos = 0;
    while(is && (pos < size)) {
        is.read(data + pos, size - pos);
        pos += is.gcount();
    }
    return pos == size;
}

template<class T>
bool ReadFromStream(std::istream& is, T& v) {
    return ReadFixFromStream(is, (char*)&v, sizeof(v));
}

template<class T>
bool ReadFromStream(std::istream& is, std::vector<T>& v) {
    return ReadFixFromStream(is, (char*)&v[0], sizeof(T) * v.size());
}

template<class T>
bool WriteToStream(std::ostream& os, const T& v) {
    if(!os) {
        return false;
    }
    os.write((const char*)&v, sizeof(T));
    return (bool)os;
}

template<class T>
bool WriteToStream(std::ostream& os, const std::vector<T>& v) {
    if(!os) {
        return false;
    }
    os.write((const char*)&v[0], sizeof(T) * v.size());
    return (bool)os;
}

class SpeedLimit {
public:
    typedef std::shared_ptr<SpeedLimit> ptr;
    SpeedLimit(uint32_t speed);
    void add(uint32_t v);
private:
    uint32_t m_speed;
    float m_countPerMS;

    uint32_t m_curCount;
    uint32_t m_curSec;
};

bool ReadFixFromStreamWithSpeed(std::istream& is, char* data,
                    const uint64_t& size, const uint64_t& speed = -1);

bool WriteFixToStreamWithSpeed(std::ostream& os, const char* data,
                            const uint64_t& size, const uint64_t& speed = -1);

template<class T>
bool WriteToStreamWithSpeed(std::ostream& os, const T& v,
                            const uint64_t& speed = -1) {
    if(os) {
        return WriteFixToStreamWithSpeed(os, (const char*)&v, sizeof(T), speed);
    }
    return false;
}

template<class T>
bool WriteToStreamWithSpeed(std::ostream& os, const std::vector<T>& v,
                            const uint64_t& speed = -1,
                            const uint64_t& min_duration_ms = 10) {
    if(os) {
        return WriteFixToStreamWithSpeed(os, (const char*)&v[0], sizeof(T) * v.size(), speed);
    }
    return false;
}

template<class T>
bool ReadFromStreamWithSpeed(std::istream& is, const std::vector<T>& v,
                            const uint64_t& speed = -1) {
    if(is) {
        return ReadFixFromStreamWithSpeed(is, (char*)&v[0], sizeof(T) * v.size(), speed);
    }
    return false;
}

template<class T>
bool ReadFromStreamWithSpeed(std::istream& is, const T& v,
                            const uint64_t& speed = -1) {
    if(is) {
        return ReadFixFromStreamWithSpeed(is, (char*)&v, sizeof(T), speed);
    }
    return false;
}

std::string Format(const char* fmt, ...);
std::string Formatv(const char* fmt, va_list ap);

template<class T>
void Slice(std::vector<std::vector<T> >& dst, const std::vector<T>& src, size_t size) {
    size_t left = src.size();
    size_t pos = 0;
    while(left > size) {
        std::vector<T> tmp;
        tmp.reserve(size);
        for(size_t i = 0; i < size; ++i) {
            tmp.push_back(src[pos + i]);
        }
        pos += size;
        left -= size;
        dst.push_back(tmp);
    }

    if(left > 0) {
        std::vector<T> tmp;
        tmp.reserve(left);
        for(size_t i = 0; i < left; ++i) {
            tmp.push_back(src[pos + i]);
        }
        dst.push_back(tmp);
    }
}


}

#endif
