#ifndef __SYLAR_HTTP_SESSION_DATA_H__
#define __SYLAR_HTTP_SESSION_DATA_H__

#include "sylar/mutex.h"
#include "sylar/singleton.h"
#include <boost/any.hpp>
#include <unordered_map>

namespace sylar {
namespace http {

class SessionData {
public:
    typedef std::shared_ptr<SessionData> ptr;
    SessionData(bool auto_gen = false);

    template<class T>
    void setData(const std::string& key, const T& v) {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_datas[key] = v;
    }

    template<class T>
    T getData(const std::string& key, const T& def = T()) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        auto it = m_datas.find(key);
        if(it == m_datas.end()) {
            return def;
        }
        boost::any v = it->second;
        lock.unlock();
        try {
            return boost::any_cast<T>(v);
        } catch (...) {
        }
        return def;
    }

    void del(const std::string& key);

    bool has(const std::string& key);
    uint64_t getLastAccessTime() const { return m_lastAccessTime;}
    void setLastAccessTime(uint64_t v) { m_lastAccessTime = v;}

    const std::string& getId() const { return m_id;}
    void setId(const std::string& val) { m_id = val;}
private:
    sylar::RWMutex m_mutex;
    std::unordered_map<std::string, boost::any> m_datas;
    uint64_t m_lastAccessTime;
    std::string m_id;
};

class SessionDataManager {
public:
    void add(SessionData::ptr info);
    void del(const std::string& id);
    SessionData::ptr get(const std::string& id);
    void check(int64_t ts = 3600);
private:
    sylar::RWMutex m_mutex;
    std::unordered_map<std::string, SessionData::ptr> m_datas;
};

typedef sylar::Singleton<SessionDataManager> SessionDataMgr;

}
}

#endif
