#ifndef __SYLAR_DB_TAIR_H__
#define __SYLAR_DB_TAIR_H__

#include <memory>
#include <map>
#include "sylar/mutex.h"
#include "sylar/fiber.h"
#include "sylar/iomanager.h"
#include "sylar/singleton.h"

namespace tair {
    class tair_client_api;
    namespace common {
        class data_entry;
    }
}

namespace sylar {

class Tair {
public:
    typedef std::shared_ptr<Tair> ptr;
public:
    Tair();
    ~Tair();

    bool init(const std::map<std::string, std::string>& confs);
    bool startup(const char* master_addr, const char* slave_addr, const char* group_name);
    bool startup();

    int32_t get(const std::string& key, std::string& val, int area, int timeout_ms = -1);
    int32_t put(const std::string& key, const std::string& val, int area, int expired = 0, int version = 0);
    int32_t remove(const std::string& key, int area);

    void setLogLevel(const std::string& level) { m_logLevel = level;}
    void setLogFile(const std::string& logfile) { m_logFile = logfile;}
    void setTimeout(int timeout) { m_timeout = timeout;}
    void setThreadCount(uint32_t thread_count) { m_threadCount = thread_count;}

    const std::string& getLogLevel() const { return m_logLevel;}
    const std::string& getLogFile() const { return m_logFile;}
    int getTimeout() const { return m_timeout;}
    uint32_t getThreadCount() const { return m_threadCount;}

    const std::string& getMasterAddr() const { return m_masterAddr;}
    const std::string& getSlaveAddr() const { return m_slaveAddr;}
    const std::string& getGroupName() const { return m_groupName;}

    void setMasterAddr(const std::string& v) { m_masterAddr = v;}
    void setSlaveAddr(const std::string& v) { m_slaveAddr = v;}
    void setGroupName(const std::string& v) { m_groupName =v;}

    void setupCache(int area, size_t capacity, uint64_t expired_time);

    const std::string toString();
private:
    void setupCacheNolock(int area, size_t capacity, uint64_t expired_time);
private:
    struct Ctx {
        typedef std::shared_ptr<Ctx> ptr;
        typedef std::weak_ptr<Ctx> weak_ptr;
        Ctx() : result(0), sn(0), start(0) {}

        int result;
        uint32_t sn;
        uint64_t start;

        std::string key;
        std::string val;
        sylar::Fiber::ptr fiber;
        sylar::Scheduler* scheduler;
        Tair* client;
        sylar::Timer::ptr timer;

        std::string toString() const;
    };
    void onTimer(Ctx::ptr ctx);
private:
    static void OnGetCb(int ret, const tair::common::data_entry *key, const tair::common::data_entry *value, void *args);
    static void OnRemoveCb(int ret, void *args);
    static void OnPutCb(int ret, void *args);
private:
    tair::tair_client_api* m_client;
    uint32_t m_sn;
    uint32_t m_timeout;
    uint32_t m_threadCount;
    std::string m_logLevel;
    std::string m_logFile;
    std::string m_masterAddr;
    std::string m_slaveAddr;
    std::string m_groupName;
    sylar::Mutex m_mutex;
    std::map<uint32_t, Ctx::ptr> m_ctxs;
    std::map<std::string, std::string> m_confs;
    std::map<uint32_t, std::pair<size_t, uint64_t> > m_caches;
};

class TairManager {
public:
    TairManager();

    Tair::ptr get(const std::string& name);
    std::ostream& dump(std::ostream& os);
private:
    void init();
private:
    sylar::RWMutex m_mutex;
    uint64_t m_idx;
    std::map<std::string, std::vector<Tair::ptr> > m_datas;
    std::map<std::string, std::map<std::string, std::string> > m_config;
};

typedef sylar::Singleton<TairManager> TairMgr;

class TairUtil {
public:
    static int32_t Get(const std::string& name, const std::string& key, std::string& val, int area, int timeout_ms = -1);
    static int32_t Put(const std::string& name, const std::string& key, const std::string& val, int area, int expired = 0, int version = 0);
    static int32_t Remove(const std::string& name, const std::string& key, int area);
    static int32_t TryGet(const std::string& name, uint32_t try_count, const std::string& key, std::string& val, int area, int timeout_ms = -1);
    static int32_t TryPut(const std::string& name, uint32_t try_count, const std::string& key, const std::string& val, int area, int expired = 0, int version = 0);
    static int32_t TryRemove(const std::string& name, uint32_t try_count, const std::string& key, int area);
};

}

#endif
