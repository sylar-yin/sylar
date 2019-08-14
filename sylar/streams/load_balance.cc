#include "load_balance.h"
#include "sylar/log.h"
#include "sylar/worker.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void LoadBalanceItem::close() {
    if(m_stream) {
        auto stream = m_stream;
        sylar::WorkerMgr::GetInstance()->schedule("service_io", [stream](){
            stream->close();
        });
    }
}

bool LoadBalanceItem::isValid() {
    return m_stream && m_stream->isConnected();
}

LoadBalanceItem::ptr LoadBalance::getById(uint64_t id) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_datas.find(id);
    return it == m_datas.end() ? nullptr : it->second;
}

void LoadBalance::add(LoadBalanceItem::ptr v) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[v->getId()] = v;
    initNolock();
}

void LoadBalance::del(LoadBalanceItem::ptr v) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.erase(v->getId());
    initNolock();
}

void LoadBalance::update(const std::unordered_map<uint64_t, LoadBalanceItem::ptr>& adds
                        ,std::unordered_map<uint64_t, LoadBalanceItem::ptr>& dels) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto& i : dels) {
        auto it = m_datas.find(i.first);
        if(it != m_datas.end()) {
            i.second = it->second;
            m_datas.erase(it);
        }
    }
    for(auto& i : adds) {
        m_datas[i.first] = i.second;
    }
    initNolock();
}

void LoadBalance::set(const std::vector<LoadBalanceItem::ptr>& vs) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.clear();
    for(auto& i : vs){
        m_datas[i->getId()] = i;
    }
    initNolock();
}

void LoadBalance::init() {
    RWMutexType::WriteLock lock(m_mutex);
    initNolock();
}

void RoundRobinLoadBalance::initNolock() {
    decltype(m_items) items;
    for(auto& i : m_datas){
        if(i.second->isValid()) {
            items.push_back(i.second);
        }
    }
    items.swap(m_items);
}

LoadBalanceItem::ptr RoundRobinLoadBalance::get(uint64_t v) {
    RWMutexType::ReadLock lock(m_mutex);
    if(m_items.empty()) {
        return nullptr;
    }
    uint32_t r = (v == (uint64_t)-1 ? rand() : v) % m_items.size();
    for(size_t i = 0; i < m_items.size(); ++i) {
        auto& h = m_items[(r + i) % m_items.size()];
        if(h->isValid()) {
            return h;
        }
    }
    return nullptr;
}

FairLoadBalanceItem::ptr WeightLoadBalance::getAsFair() {
    auto item = get();
    if(item) {
        return std::static_pointer_cast<FairLoadBalanceItem>(item);
    }
    return nullptr;
}

LoadBalanceItem::ptr WeightLoadBalance::get(uint64_t v) {
    RWMutexType::ReadLock lock(m_mutex);
    int32_t idx = getIdx(v);
    if(idx == -1) {
        return nullptr;
    }

    //TODO fix weight
    for(size_t i = 0; i < m_items.size(); ++i) {
        auto& h = m_items[(idx + i) % m_items.size()];
        if(h->isValid()) {
            return h;
        }
    }
    return nullptr;
}

void WeightLoadBalance::initNolock() {
    decltype(m_items) items;
    for(auto& i : m_datas){
        if(i.second->isValid()) {
            items.push_back(i.second);
        }
    }
    items.swap(m_items);

    int32_t total = 0;
    m_weights.resize(m_items.size());
    for(size_t i = 0; i < m_items.size(); ++i) {
        total += m_items[i]->getWeight();
        m_weights[i] = total;
    }
}

int32_t WeightLoadBalance::getIdx(uint64_t v) {
    if(m_weights.empty()) {
        return -1;
    }
    int32_t total = *m_weights.rbegin();
    auto it = std::upper_bound(m_weights.begin()
                ,m_weights.end(), (v == (uint64_t)-1 ? rand() : v) % total);
    return std::distance(it, m_weights.begin());
}

void HolderStats::clear() {
    m_usedTime = 0;
    m_total = 0;
    m_doing = 0;
    m_timeouts = 0;
    m_oks = 0;
    m_errs = 0;
}

float HolderStats::getWeight(float rate) {
    if(m_total == 0) {
        return 0.5;
    }
    float base = m_total + 10;
    static const uint32_t MAX_TS = 1000;
    float avg_time = (m_oks == 0 ? 0 : m_usedTime / m_oks) * 1.0 / MAX_TS;
    return (1 - avg_time)
        //* (m_oks / base)
        * (1 - m_timeouts / base * 0.5) 
        * (1 - m_doing / base * 0.1)
        * (1 - m_errs / base) * rate;
}

HolderStatsSet::HolderStatsSet(uint32_t size) {
    m_stats.resize(size);
}

void HolderStatsSet::init(const uint32_t& now) {
    if(m_lastUpdateTime < now) {
        for(uint32_t t = m_lastUpdateTime + 1, i = 0;
                t <= now && i < m_stats.size(); ++t, ++i) {
            m_stats[t % m_stats.size()].clear();
        }
        m_lastUpdateTime = now;
    }
}

HolderStats& HolderStatsSet::get(const uint32_t& now) {
    init(now);
    return m_stats[now % m_stats.size()];
}

float HolderStatsSet::getWeight(const uint32_t& now) {
    init(now);
    float v = 0;
    for(size_t i = 0; i < m_stats.size(); ++i) {
        v += m_stats[(now + i) % m_stats.size()].getWeight(1 - 0.1 * i);
    }
    return v;
}

int32_t FairLoadBalanceItem::getWeight() {
    int32_t v = m_weight * m_stats.getWeight();
    if(m_stream->isConnected()) {
        return std::max(1, v);
    }
    return 1;
}

HolderStats& LoadBalanceItem::get(const uint32_t& now) {
    return m_stats.get(now);
}

//LoadBalanceItem::ptr FairLoadBalance::get() {
//    RWMutexType::ReadLock lock(m_mutex);
//    int32_t idx = getIdx();
//    if(idx == -1) {
//        return nullptr;
//    }
//
//    //TODO fix weight
//    for(size_t i = 0; i < m_items.size(); ++i) {
//        auto& h = m_items[(idx + i) % m_items.size()];
//        if(h->isValid()) {
//            return h;
//        }
//    }
//    return nullptr;
//}
//
//void FairLoadBalance::initNolock() {
//    decltype(m_items) items;
//    for(auto& i : m_datas){
//        items.push_back(i.second);
//    }
//    items.swap(m_items);
//
//    m_weights.resize(m_items.size());
//    int32_t total = 0;
//    for(size_t i = 0; i < m_items.size(); ++i) {
//        total += m_items[i]->getWeight();
//        m_weights[i] = total;
//    }
//}
//
//int32_t FairLoadBalance::getIdx() {
//    if(m_weights.empty()) {
//        return -1;
//    }
//    int32_t total = *m_weights.rbegin();
//    auto it = std::upper_bound(m_weights.begin()
//                ,m_weights.end(), rand() % total);
//    return std::distance(it, m_weights.begin());
//}

SDLoadBalance::SDLoadBalance(IServiceDiscovery::ptr sd)
    :m_sd(sd) {
}

LoadBalance::ptr SDLoadBalance::get(const std::string& domain, const std::string& service, bool auto_create) {
    do {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_datas.find(domain);
        if(it == m_datas.end()) {
            break;
        }
        auto iit = it->second.find(service);
        if(iit == it->second.end()) {
            break;
        }
        return iit->second;
    } while(0);

    if(!auto_create) {
        return nullptr;
    }

    auto type = getType(domain, service);

    auto lb = createLoadBalance(type);
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[domain][service] = lb;
    lock.unlock();
    return lb;
}


ILoadBalance::Type SDLoadBalance::getType(const std::string& domain, const std::string& service) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_types.find(domain);
    if(it == m_types.end()) {
        return m_defaultType;
    }
    auto iit = it->second.find(service);
    if(iit == it->second.end()) {
        return m_defaultType;
    }
    return iit->second;
}

LoadBalance::ptr SDLoadBalance::createLoadBalance(ILoadBalance::Type type) {
    if(type == ILoadBalance::ROUNDROBIN) {
        return RoundRobinLoadBalance::ptr(new RoundRobinLoadBalance);
    } else if(type == ILoadBalance::WEIGHT) {
        return WeightLoadBalance::ptr(new WeightLoadBalance);
    } else if(type == ILoadBalance::FAIR) {
        return WeightLoadBalance::ptr(new WeightLoadBalance);
    }
    return nullptr;
}

LoadBalanceItem::ptr SDLoadBalance::createLoadBalanceItem(ILoadBalance::Type type) {
    LoadBalanceItem::ptr item;
    if(type == ILoadBalance::ROUNDROBIN) {
        item.reset(new LoadBalanceItem);
    } else if(type == ILoadBalance::WEIGHT) {
        item.reset(new LoadBalanceItem);
    } else if(type == ILoadBalance::FAIR) {
        item.reset(new FairLoadBalanceItem);
    }
    return item;
}

void SDLoadBalance::onServiceChange(const std::string& domain, const std::string& service
                            ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
                            ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value) {
    auto type = getType(domain, service);
    auto lb = get(domain, service, true);
    std::unordered_map<uint64_t, ServiceItemInfo::ptr> add_values;
    std::unordered_map<uint64_t, LoadBalanceItem::ptr> del_infos;

    for(auto& i : old_value) {
        if(new_value.find(i.first) == new_value.end()) {
            del_infos[i.first];
        }
    }
    for(auto& i : new_value) {
        if(old_value.find(i.first) == old_value.end()) {
            add_values.insert(i);
        }
    }

    std::unordered_map<uint64_t, LoadBalanceItem::ptr> add_infos;
    for(auto& i : add_values) {
        auto stream = m_cb(i.second);
        if(!stream) {
            SYLAR_LOG_ERROR(g_logger) << "create stream fail, " << i.second->toString();
            continue;
        }
        
        LoadBalanceItem::ptr lditem = createLoadBalanceItem(type);
        lditem->setStream(stream);
        lditem->setWeight(10000);

        add_infos[i.first] = lditem;
    }

    lb->update(add_infos, del_infos);
    for(auto& i : del_infos) {
        if(i.second) {
            i.second->close();
        }
    }
}

void SDLoadBalance::start() {
    m_sd->setServiceCallback(std::bind(&SDLoadBalance::onServiceChange, this
                ,std::placeholders::_1
                ,std::placeholders::_2
                ,std::placeholders::_3
                ,std::placeholders::_4));
    m_sd->start();
}

void SDLoadBalance::stop() {
    m_sd->stop();
}

void SDLoadBalance::initConf(const std::unordered_map<std::string
                            ,std::unordered_map<std::string,std::string> >& confs) {
    decltype(m_types) types;
    std::unordered_map<std::string, std::unordered_set<std::string> > query_infos;
    for(auto& i : confs) {
        for(auto& n : i.second) {
            ILoadBalance::Type t = ILoadBalance::FAIR;
            if(n.second == "round_robin") {
                t = ILoadBalance::ROUNDROBIN;
            } else if(n.second == "weight") {
                t = ILoadBalance::WEIGHT;
            }
            types[i.first][n.first] = t;
            query_infos[i.first].insert(n.first);
        }
    }
    m_sd->setQueryServer(query_infos);
    RWMutexType::WriteLock lock(m_mutex);
    types.swap(m_types);
    lock.unlock();
}

}
