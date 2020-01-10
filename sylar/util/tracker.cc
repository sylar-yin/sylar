#include "tracker.h"
#include "sylar/worker.h"
#include "sylar/util.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Tracker::Tracker()
    :m_datas(nullptr)
    ,m_interval(60) {
}

Tracker::~Tracker() {
    if(m_datas) {
        delete[] m_datas;
    }
}

void Tracker::init() {
    if(m_datas) {
        delete[] m_datas;
        m_datas = nullptr;
    }
    m_dimCount = m_dims.size() * m_interval;
    m_datas = new int64_t[m_dimCount]();
}

void Tracker::start() {
    if(m_timer) {
        return;
    }
    init();
    auto iom = sylar::WorkerMgr::GetInstance()->getAsIOManager("timer");
    if(iom) {
        m_timer = iom->addTimer(100, std::bind(&Tracker::onTimer, shared_from_this()), true);
    } else {
        SYLAR_LOG_INFO(g_logger) << "Woker timer not exists";
        sylar::IOManager::GetThis()->addTimer(100, std::bind(&Tracker::onTimer, shared_from_this()), true);
    }
}

void Tracker::stop() {
    if(m_timer) {
        m_timer->cancel();
        m_timer = nullptr;
    }
}

int64_t Tracker::inc(uint32_t key, int64_t v) {
    if(!m_datas) {
        return 0;
    }
    time_t now = time(0);
    auto idx = key * m_interval + now % m_interval;
    if(idx < m_dimCount) {
        return sylar::Atomic::addFetch(m_datas[idx], v);
    }
    return 0;
}

int64_t Tracker::dec(uint32_t key, int64_t v) {
    if(!m_datas) {
        return 0;
    }
    time_t now = time(0);
    auto idx = key * m_interval + now % m_interval;
    if(idx < m_dimCount) {
        return sylar::Atomic::subFetch(m_datas[idx], v);
    }
    return 0;
}

int64_t Tracker::privateInc(uint32_t key, int64_t v) {
    if(!m_datas) {
        return 0;
    }
    if(key < m_dimCount) {
        return sylar::Atomic::subFetch(m_datas[key], v);
    }
    return 0;
}

int64_t Tracker::privateDec(uint32_t key, int64_t v) {
    if(!m_datas) {
        return 0;
    }
    if(key < m_dimCount) {
        return sylar::Atomic::subFetch(m_datas[key], v);
    }
    return 0;
}

void Tracker::toData(std::map<std::string, int64_t>& data, const std::set<uint32_t>& times) {
    time_t now = time(0) - 1;
    for(uint32_t i = 0; i < m_dims.size(); ++i) {
        int64_t total = 0;
        if(m_dims[i].empty()) {
            continue;
        }
        for(uint32_t n = 0; n < m_interval; ++n) {
            int32_t cur = i * m_interval + (now - n) % m_interval;
            total += m_datas[cur];

            if(times.count(n + 1)) {
                data[m_dims[i] + "." + std::to_string(n + 1)] = total;
            }
        }
        if(times.empty()) {
            data[m_dims[i] + "." + std::to_string(m_interval)] = total;
        }
    }
}

void Tracker::toDurationData(std::map<std::string, int64_t>& data, uint32_t duration, bool with_time) {
    time_t now = time(0) - 1;
    if(duration > m_interval) {
        duration = m_interval;
    }
    for(uint32_t i = 0; i < m_dims.size(); ++i) {
        int64_t total = 0;
        if(m_dims[i].empty()) {
            continue;
        }
        for(uint32_t n = 0; n < duration; ++n) {
            int32_t cur = i * m_interval + (now - n) % m_interval;
            total += m_datas[cur];
        }
        if(with_time) {
            data[m_dims[i] + "." + std::to_string(duration)] = total;
        } else {
            data[m_dims[i]] = total;
        }
    }
}

void Tracker::toTimeData(std::map<std::string, int64_t>& data, uint32_t ts, bool with_time) {
    if(ts >= m_interval) {
        return;
    }
    for(uint32_t i = 0; i < m_dims.size(); ++i) {
        if(m_dims[i].empty()) {
            continue;
        }
        int32_t cur = i * m_interval + ts;
        if(with_time) {
            data[m_dims[i] + "." + std::to_string(ts)] = m_datas[cur];
        } else {
            data[m_dims[i]] = m_datas[cur];
        }
    }
}

void Tracker::addDim(uint32_t key, const std::string& name) {
    if(m_timer) {
        SYLAR_LOG_ERROR(g_logger) << "Tracker is running, addDim key=" << key << " name=" << name;
        return;
    }
    if(key >= m_dims.size()) {
        m_dims.resize(key + 1);
    }
    m_dims[key] = name;
}

void Tracker::onTimer() {
    time_t now = time(0);
    int offset = (now + 1) % m_interval;

    for(uint32_t i = 0; i < m_dims.size(); ++i) {
        auto idx = i * m_interval + offset;
        if(idx < m_dimCount) {
            m_datas[idx] = 0;
        }
    }
}

TrackerManager::TrackerManager()
    :m_interval(60) {
}

void TrackerManager::addDim(uint32_t key, const std::string& name) {
    RWMutexType::WriteLock lock(m_mutex);
    if(key >= m_dims.size()) {
        m_dims.resize(key + 1);
    }
    m_dims[key] = name;
}

int64_t TrackerManager::inc(const std::string& name, uint32_t key, int64_t v) {
    m_totalTracker->inc(key, v);
    auto info = getOrCreate(name);
    return info->inc(key, v);
}

int64_t TrackerManager::dec(const std::string& name, uint32_t key, int64_t v) {
    m_totalTracker->dec(key, v);
    auto info = getOrCreate(name);
    return info->dec(key, v);
}

Tracker::ptr TrackerManager::getOrCreate(const std::string& name) {
    do {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_datas.find(name);
        if(it != m_datas.end()) {
            return it->second;
        }
    } while(0);
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_datas.find(name);
    if(it != m_datas.end()) {
        return it->second;
    }

    Tracker::ptr rt = std::make_shared<Tracker>();
    rt->m_dims = m_dims;
    rt->m_interval = m_interval;
    rt->m_name = name;
    rt->start();
    m_datas[name] = rt;
    lock.unlock();
    return rt;
}

Tracker::ptr TrackerManager::get(const std::string& name) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(name);
    return it == m_datas.end() ? nullptr : it->second;
}

std::string TrackerManager::toString(uint32_t duration) {
    std::stringstream ss;
    RWMutexType::ReadLock lock(m_mutex);
    auto datas = m_datas;
    lock.unlock();

    ss << "<TrackerManager count=" << datas.size() << ">" << std::endl;
    if(m_totalTracker) {
        std::map<std::string, int64_t> data;
        m_totalTracker->toDurationData(data, duration);

        ss << "[Tracker name=" << m_totalTracker->m_name << "]" << std::endl;
        for(auto& n : data) {
            ss << std::setw(40) << std::right << n.first << ": " << n.second << std::endl;
        }
    }
    for(auto& i : datas) {
        std::map<std::string, int64_t> data;
        i.second->toDurationData(data, duration);

        ss << "[Tracker name=" << i.first << "]" << std::endl;
        for(auto& n : data) {
            ss << std::setw(40) << std::right << n.first << ": " << n.second << std::endl;
        }
    }
    return ss.str();
}

void TrackerManager::start() {
    if(m_timer) {
        return;
    }

    m_totalTracker = std::make_shared<Tracker>();
    m_totalTracker->m_dims = m_dims;
    m_totalTracker->m_interval = 1;
    m_totalTracker->m_name = "total";
    m_totalTracker->init();

    m_timer = sylar::IOManager::GetThis()->addTimer(1000 * 60 * 60 * 8, [this](){
            RWMutexType::WriteLock lock(m_mutex);
            auto datas = m_datas;
            m_datas.clear();
            lock.unlock();

            for(auto& i : datas) {
                i.second->stop();
            }
    }, true);
}

}
