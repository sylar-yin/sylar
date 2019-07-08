#include "load_balance.h"

namespace sylar {

void LoadBalance::add(LoadBalanceItem::ptr v) {
    RWMutexType::WriteLock lock(m_mutex);
    m_items.push_back(v);
    //TODO init
}

void LoadBalance::del(LoadBalanceItem::ptr v) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_items.begin();
            it != m_items.end(); ++it) {
        if(*it == v) {
            m_items.erase(it);
            //TODO uninit
            break;
        }
    }
}

void LoadBalance::set(const std::vector<LoadBalanceItem::ptr>& vs) {
    RWMutexType::WriteLock lock(m_mutex);
    m_items = vs;
    //TODO init
}

LoadBalanceItem::ptr RoundRobinLoadBalance::get() {
    RWMutexType::ReadLock lock(m_mutex);
    if(m_items.empty()) {
        return nullptr;
    }
    uint32_t r = rand() % m_items.size();
    for(size_t i = 0; i < m_items.size(); ++i) {
        auto& h = m_items[(r + i) % m_items.size()];
        if(h->isValid()) {
            return h;
        }
    }
    return nullptr;
}

LoadBalanceItem::ptr WeightLoadBalance::get() {
    RWMutexType::ReadLock lock(m_mutex);
    int32_t idx = getIdx();
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

int32_t WeightLoadBalance::initWeight() {
    int32_t total = 0;
    m_weights.resize(m_items.size());
    for(size_t i = 0; i < m_items.size(); ++i) {
        total += m_items[i]->getWeight();
        m_weights[i] = total;
    }
    return total;
}

int32_t WeightLoadBalance::getIdx() {
    if(m_weights.empty()) {
        return -1;
    }
    int32_t total = *m_weights.rbegin();
    auto it = std::upper_bound(m_weights.begin()
                ,m_weights.end(), rand() % total);
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

LoadBalanceItem::ptr FairLoadBalance::get() {
    RWMutexType::ReadLock lock(m_mutex);
    int32_t idx = getIdx();
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

FairLoadBalanceItem::ptr FairLoadBalance::getAsFair() {
    auto item = get();
    if(item) {
        return std::static_pointer_cast<FairLoadBalanceItem>(item);
    }
    return nullptr;
}

HolderStats& FairLoadBalanceItem::get(const uint32_t& now) {
    return m_stats.get(now);
}

int32_t FairLoadBalance::initWeight() {
    m_weights.resize(m_items.size());
    int32_t total = 0;
    for(size_t i = 0; i < m_items.size(); ++i) {
        total += m_items[i]->getWeight();
        m_weights[i] = total;
    }
    return total;
}

int32_t FairLoadBalance::getIdx() {
    if(m_weights.empty()) {
        return -1;
    }
    int32_t total = *m_weights.rbegin();
    auto it = std::upper_bound(m_weights.begin()
                ,m_weights.end(), rand() % total);
    return std::distance(it, m_weights.begin());
}

}
