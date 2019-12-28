#ifndef __SYLAR_DS_TIMED_CACHE_H__
#define __SYLAR_DS_TIMED_CACHE_H__

#include "cache_status.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include <set>
#include <unordered_map>

namespace sylar {
namespace ds {

template<class K, class V, class RWMutexType = sylar::RWMutex>
class TimedCache {
private:
    struct Item {
        Item(const K& k, const V& v, const uint64_t& t)
            :key(k), val(v), ts(t) { }
        K key;
        mutable V val;
        uint64_t ts;

        bool operator< (const Item& oth) const {
            if(ts != oth.ts) {
                return ts < oth.ts;
            }
            return key < oth.key;
        }
    };
public:
    typedef std::shared_ptr<TimedCache> ptr;
    typedef Item item_type;
    typedef std::set<item_type> set_type;
    typedef std::unordered_map<K, typename set_type::iterator> map_type;
    typedef std::function<void(const K&, const V&)> prune_callback;

    TimedCache(size_t max_size = 0, size_t elasticity = 0
               , CacheStatus* status = nullptr)
        :m_maxSize(max_size)
        ,m_elasticity(elasticity)
        ,m_status(status) {
        if(m_status == nullptr) {
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }

    ~TimedCache() {
        if(m_statusOwner && m_status) {
            delete m_status;
        }
    }

    void set(const K& k, const V& v, uint64_t expired) {
        m_status->incSet();
        typename RWMutexType::WriteLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it != m_cache.end()) {
            m_timed.erase(it->second);
            m_cache.erase(it);
        }
        auto sit = m_timed.insert(Item(k, v, expired + sylar::GetCurrentMS()));
        m_cache.insert(std::make_pair(k, sit.first));
        prune();
    }

    bool get(const K& k, V& v) {
        m_status->incGet();
        typename RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return false;
        }
        v = it->second->val;
        lock.unlock();
        m_status->incHit();
        return true;
    }

    V get(const K& k) {
        m_status->incGet();
        typename RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return V();
        }
        auto v = it->second->val;
        lock.unlock();
        m_status->incHit();
        return v;
    }

    bool del(const K& k) {
        m_status->incDel();
        typename RWMutexType::WriteLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return false;
        }
        m_timed.erase(it->second);
        m_cache.erase(it);
        lock.unlock();
        m_status->incHit();
    }

    bool expired(const K& k, const uint64_t& ts) {
        typename RWMutexType::WriteLock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return false;
        }
        uint64_t tts = ts + sylar::GetCurrentMS();
        if(it->second->ts == tts) {
            return true;
        }
        auto item = *it->second;
        item.ts = tts;
        m_timed.erase(it->second);
        auto iit = m_timed.insert(item);
        it->second = iit.first;
        return true;
    }

    bool exists(const K& k) {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.find(k) != m_cache.end();
    }

    size_t size() {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.size();
    }

    size_t empty() {
        typename RWMutexType::ReadLock lock(m_mutex);
        return m_cache.empty();
    }

    bool clear() {
        typename RWMutexType::WriteLock lock(m_mutex);
        m_timed.clear();
        m_cache.clear();
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

    size_t checkTimeout(const uint64_t& ts = sylar::GetCurrentMS()) {
        size_t size = 0;
        typename RWMutexType::WriteLock lock(m_mutex);
        for(auto it = m_timed.begin();
                it != m_timed.end();) {
            if(it->ts <= ts) {
                if(m_cb) {
                    m_cb(it->key, it->val);
                }
                m_cache.erase(it->key);
                m_timed.erase(it++);
                ++size;
            } else {
                break;
            }
        }
        return size;
    }
protected:
    size_t prune() {
        if(m_maxSize == 0 || m_cache.size() < getMaxAllowedSize()) {
            return 0;
        }
        size_t count = 0;
        while(m_cache.size() > m_maxSize) {
            auto it = m_timed.begin();
            if(m_cb) {
                m_cb(it->key, it->val);
            }
            m_cache.erase(it->key);
            m_timed.erase(it);
            ++count;
        }
        m_status->incPrune(count);
        return count;
    }
private:
    RWMutexType m_mutex;
    uint64_t m_maxSize;
    uint64_t m_elasticity;
    CacheStatus* m_status;
    map_type m_cache;
    set_type m_timed;
    prune_callback m_cb;
    bool m_statusOwner = false;
};

template<class K, class V, class RWMutexType = sylar::RWMutex, class Hash = std::hash<K> >
class HashTimedCache {
public:
    typedef std::shared_ptr<HashTimedCache> ptr;
    typedef TimedCache<K, V, RWMutexType> cache_type;

    HashTimedCache(size_t bucket, size_t max_size, size_t elasticity)
        :m_bucket(bucket) {
        m_datas.resize(bucket);

        size_t pre_max_size = std::ceil(max_size * 1.0 / bucket);
        size_t pre_elasiticity = std::ceil(elasticity * 1.0 / bucket);
        m_maxSize = pre_max_size * bucket;
        m_elasticity = pre_elasiticity * bucket;

        for(size_t i = 0; i < bucket; ++i) {
            m_datas[i] = new cache_type(pre_max_size
                        ,pre_elasiticity, &m_status);
        }
    }

    ~HashTimedCache() {
        for(size_t i = 0; i < m_datas.size(); ++i) {
            delete m_datas[i];
        }
    }

    void set(const K& k, const V& v, uint64_t expired) {
        m_datas[m_hash(k) % m_bucket]->set(k, v, expired);
    }

    bool expired(const K& k, const uint64_t& ts) {
        return m_datas[m_hash(k) % m_bucket]->expired(k, ts);
    }

    bool get(const K& k, V& v) {
        return m_datas[m_hash(k) % m_bucket]->get(k, v);
    }

    V get(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->get(k);
    }

    bool del(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->del(k);
    }

    bool exists(const K& k) {
        return m_datas[m_hash(k) % m_bucket]->exists(k);
    }

    size_t size() {
        size_t total = 0;
        for(auto& i : m_datas) {
            total += i->size();
        }
        return total;
    }

    bool empty() {
        for(auto& i : m_datas) {
            if(!i->empty()) {
                return false;
            }
        }
        return true;
    }

    void clear() {
        for(auto& i : m_datas) {
            i->clear();
        }
    }

    size_t getMaxSize() const { return m_maxSize;}
    size_t getElasticity() const { return m_elasticity;}
    size_t getMaxAllowedSize() const { return m_maxSize + m_elasticity;}
    size_t getBucket() const { return m_bucket;}

    void setMaxSize(const size_t& v) {
        size_t pre_max_size = std::ceil(v * 1.0 / m_bucket);
        m_maxSize = pre_max_size * m_bucket;
        for(auto& i : m_datas) {
            i->setMaxSize(pre_max_size);
        }
    }

    void setElasticity(const size_t& v) {
        size_t pre_elasiticity = std::ceil(v * 1.0 / m_bucket);
        m_elasticity = pre_elasiticity * m_bucket;
        for(auto& i : m_datas) {
            i->setElasticity(pre_elasiticity);
        }
    }

    template<class F>
    void foreach(F& f) {
        for(auto& i : m_datas) {
            i->foreach(f);
        }
    }

    void setPruneCallback(typename cache_type::prune_callback cb) {
        for(auto& i : m_datas) {
            i->setPruneCallback(cb);
        }
    }

    CacheStatus* getStatus() {
        return &m_status;
    }

    std::string toStatusString() {
        std::stringstream ss;
        ss << m_status.toString() << " total=" << size();
        return ss.str();
    }

    size_t checkTimeout(const uint64_t& ts = sylar::GetCurrentMS()) {
        size_t size = 0;
        for(auto& i : m_datas) {
            size += i->checkTimeout(ts);
        }
        return size;
    }
private:
    std::vector<cache_type*> m_datas;
    size_t m_maxSize;
    size_t m_bucket;
    size_t m_elasticity;
    Hash m_hash;
    CacheStatus m_status;
};

}
}

#endif
