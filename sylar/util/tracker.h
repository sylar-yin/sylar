#ifndef __SYLAR_UTIL_TRACKER_H__
#define __SYLAR_UTIL_TRACKER_H__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "sylar/iomanager.h"
#include "sylar/singleton.h"

namespace sylar {

class TrackerManager;
class Tracker : public std::enable_shared_from_this<Tracker> {
friend class TrackerManager;
public:
    typedef std::shared_ptr<Tracker> ptr;
    typedef std::function<void(const std::vector<std::string>& dims
            ,int64_t* datas, int64_t interval)> callback;

    Tracker();
    ~Tracker();

    void start();
    void stop();

    int64_t inc(uint32_t key, int64_t v);
    int64_t dec(uint32_t key, int64_t v);

    void addDim(uint32_t key, const std::string& name);

    int64_t  getInterval() const { return m_interval;}
    void setInterval(int64_t v) { m_interval = v;}

    const std::string& getName() const { return m_name;}
    void setName(const std::string& v) { m_name = v;}

    void toData(std::map<std::string, int64_t>& data, const std::set<uint32_t>& times = {});
    void toDurationData(std::map<std::string, int64_t>& data, uint32_t duration = 1, bool with_time = true);
    void toTimeData(std::map<std::string, int64_t>& data, uint32_t ts = 1, bool with_time = true);
private:
    void onTimer();

    int64_t privateInc(uint32_t key, int64_t v);
    int64_t privateDec(uint32_t key, int64_t v);
    void init();
private:
    std::string m_name;
    std::vector<std::string> m_dims;
    int64_t* m_datas;
    uint32_t m_interval;
    uint32_t m_dimCount;
    sylar::Timer::ptr m_timer;
};

class TrackerManager {
public:
    typedef sylar::RWSpinlock RWMutexType;
    TrackerManager();
    void addDim(uint32_t key, const std::string& name);

    int64_t inc(const std::string& name, uint32_t key, int64_t v);
    int64_t dec(const std::string& name, uint32_t key, int64_t v);

    int64_t  getInterval() const { return m_interval;}
    void setInterval(int64_t v) { m_interval = v;}

    Tracker::ptr getOrCreate(const std::string& name);
    Tracker::ptr get(const std::string& name);

    std::string toString(uint32_t duration = 10);

    void start();
private:
    RWMutexType m_mutex;
    uint32_t m_interval;
    std::vector<std::string> m_dims;
    std::unordered_map<std::string, Tracker::ptr> m_datas;
    Tracker::ptr m_totalTracker;
    sylar::Timer::ptr m_timer;
};

typedef sylar::Singleton<TrackerManager> TrackerMgr;

}

#endif
