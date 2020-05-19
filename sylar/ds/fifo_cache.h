#ifndef __SYLAR_DS_FIFO_CACHE_H__
#define __SYLAR_DS_FIFO_CACHE_H__

namespace sylar {
namespace ds {

template<class K, class V, class Hash = std::hash<K>, class RWMutexType = sylar::RWMutex>
class FifoCache {
public:
    typedef std::pair<V, int64_t> value_type;
    typedef std::unordered_map<K, value_type> map_type;
    typedef std::list<K> list_type;
    typedef std::function<void(const K&, const V&)> prune_callback;

    FifoCache(size_t max_size = 0, size_t elasticity = 0
              ,CacheStatus* status = nullptr)
        :m_maxSize(max_size)
        ,m_elasticity(elasticity)
        ,m_status(status) {
        if(m_status == nullptr) {
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }

    ~FifoCache() {
        if(m_statusOwner && m_status) {
            delete m_status;
        }
    }

    void set(const K& k, const V& v, int64_t diff_time_s = 0) {
        m_status->incSet();
        typename RWMutexType::WriteLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it != m_cache.end()) {
            it->second.first = v;
            it->second.second = time(0) + diff_time_s;
            return;
        }
        m_cache[k] = std::make_pair(v, time(0) + diff_time_s);
        m_keys.push_back(k);
        prune();
    }

    int64_t get(const K& k, V& v) {
        m_status->incGet();
        typename RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return 0;
        }
        v = it->second.first;
        m_status->incHit();
        return it->second.second;
    }

    V get(const K& k) {
        m_status->incGet();
        typename RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it != m_cache.end()) {
            m_status->incHit();
            return it->second.first;
        }
        return nullptr;
    }

    bool exists(const K& k) {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.find(k) != m_cache.end();
    }

    size_t size() {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.size();
    }

    bool empty() {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.empty();
    }

    bool clear() {
        typename RWMutexType::ReadLock lock(m_mutex);
        m_cache.clear();
        m_keys.clear();
        return true;
    }

    void setMaxSize(const size_t& v) { m_maxSize = v;}
    void setElasticity(const size_t& v) { m_elasticity = v;}

    size_t getMaxSize() const { return m_maxSize;}
    size_t getElasticity() const { return m_elasticity;}
    size_t getMaxAllowedSize() const { return m_maxSize + m_elasticity;}

    template<class F>
    void foreach(F& f) {
        typename RWMutexType::ReadLock lock(m_mutex);
        std::for_each(m_cache.begin(), m_cache.end(), f);
    }

    void setPruneCallback(prune_callback cb) { m_cb = cb;}

    std::string toStatusString() {
        std::stringstream ss;
        ss << (m_status ? m_status->toString() : "(no status)")
           << " total=" << size();
        return ss.str();
    }

    CacheStatus* getStatus() const { return m_status;}

    void setStatus(CacheStatus* v, bool owner = false) {
        if(m_statusOwner && m_status) {
            delete m_status;
        }
        m_status = v;
        m_statusOwner = owner;

        if(m_status == nullptr) {
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }
protected:
    size_t prune() {
        if(m_maxSize == 0 || m_cache.size() < getMaxAllowedSize()) {
            return 0;
        }
        size_t count = 0;
        while(m_keys.size() > m_maxSize) {
            auto& back = m_keys.back();
            //if(m_cb) {
            //    m_cb(first);
            //}
            m_cache.erase(back);
            m_keys.pop_back();
            ++count;
        }
        m_status->incPrune(count);
        return count;
    }
private:
    RWMutexType m_mutex;
    map_type m_cache;
    list_type m_keys;
    size_t m_maxSize;
    size_t m_elasticity;
    prune_callback m_cb;
    CacheStatus* m_status = nullptr;
    bool m_statusOwner = false;
};

}
}

#endif
