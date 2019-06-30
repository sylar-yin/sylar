#ifndef __SYLAR_DS_TIMED_LRU_CACHE_H__
#define __SYLAR_DS_TIMED_LRU_CACHE_H__

#include "cache_status.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include <set>
#include <list>
#include <unordered_map>

namespace sylar {
namespace ds {

template<class K, class V, class MutexType = sylar::Mutex>
class TimedLruCache {
private:
    struct Item {
        Item(const K& k, const V& v, const uint64_t& t)
            :key(k), val(v), ts(t) { }
        K key;
        mutable V val;
        uint64_t ts;

        bool operator< (const Item& oth) const {
            return key < oth.key;
        }
    };
public:
    typedef std::shared_ptr<TimedLruCache> ptr;
    typedef Item item_type;
    typedef std::list<item_type> list_type;
    typedef typename list_type::iterator value_type;
    typedef std::unordered_map<K, value_type> map_type;
    typedef std::function<void(const K&, const V&)> prune_callback;
private:
    struct ItemTimeOp {
        bool operator()(const value_type& a
                        ,const value_type& b) const {
            if(a == b) {
                return false;
            }
            if(a->ts != b->ts) {
                return a->ts < b->ts;
            }
            return a->key < b->key;
        }
    };
public:
    typedef std::set<value_type, ItemTimeOp> set_type;

    TimedLruCache(size_t max_size = 0, size_t elasticity = 0
                  ,CacheStatus* status = nullptr)
        :m_maxSize(max_size)
        ,m_elasticity(elasticity)
        ,m_status(status) {
        if(m_status == nullptr) {
            m_status = new CacheStatus;
            m_statusOwner = true;
        }
    }

    ~TimedLruCache() {
        if(m_statusOwner && m_status) {
            delete m_status;
        }
    }

    void set(const K& k, const V& v, uint64_t expired) {
        m_status->incSet();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it != m_cache.end()) {
            m_keys.splice(m_keys.begin(), m_keys, it->second);
            m_timed.erase(it->second);
            it->second->val = v;
            it->second->ts = expired + sylar::GetCurrentMS();
            m_timed.insert(it->second);
            return;
        }

        m_keys.emplace_front(Item(k, v, expired + sylar::GetCurrentMS()));
        m_cache.insert(std::make_pair(k, m_keys.begin()));
        m_timed.insert(m_keys.begin());
        prune();
    }

    bool get(const K& k, V& v) {
        m_status->incGet();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return false;
        }
        m_keys.splice(m_keys.begin(), m_keys, it->second);
        v = it->second->val;
        lock.unlock();
        m_status->incHit();
        return true;
    }

    V get(const K& k) {
        m_status->incGet();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return V();
        }
        m_keys.splice(m_keys.begin(), m_keys, it->second);
        auto v = it->second->val;
        lock.unlock();
        m_status->incHit();
        return v;
    }

    bool del(const K& k) {
        m_status->incDel();
        typename MutexType::Lock lock(m_mutex);
        auto it = m_cache.find(k);
        if(it == m_cache.end()) {
            return false;
        }
        m_timed.erase(it->second);
        m_keys.erase(it->second);
        m_cache.erase(it);
        return true;
    }

    bool exists(const K& k) {
        typename MutexType::Lock lock(m_mutex);
        return m_cache.find(k) != m_cache.end();
    }

    size_t size() {
        typename MutexType::Lock lock(m_mutex);
        return m_cache.size();
    }

    bool empty() {
        typename MutexType::Lock lock(m_mutex);
        return m_cache.empty();
    }

    bool clear() {
        typename MutexType::Lock lock(m_mutex);
        m_cache.clear();
        m_keys.clear();
        m_timed.clear();
        return true;
    }

    void setMaxSize(const size_t& v) { m_maxSize = v;}
    void setElasticity(const size_t& v) { m_elasticity = v;}

    size_t getMaxSize() const { return m_maxSize;}
    size_t getElasticity() const { return m_elasticity;}
    size_t getMaxAllowedSize() const { return m_maxSize + m_elasticity;}

    template<class F>
    void foreach(F& f) {
        typename MutexType::Lock lock(m_mutex);
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
        typename MutexType::Lock lock(m_mutex);
        for(auto it = m_timed.begin();
                it != m_timed.end();) {
            if((*it)->ts <= ts) {
                if(m_cb) {
                    m_cb((*it)->key, (*it)->val);
                }
                m_cache.erase((*it)->key);
                m_keys.erase(*it);
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
            auto& back = m_keys.back();
            if(m_cb) {
                m_cb(back.key, back.val);
            }
            m_cache.erase(back.key);
            m_timed.erase(--m_keys.end());
            m_keys.pop_back();
            ++count;
        }
        m_status->incPrune(count);
        return count;
    }
private:
    MutexType m_mutex;
    map_type m_cache;
    list_type m_keys;
    set_type m_timed;
    size_t m_maxSize;
    size_t m_elasticity;
    prune_callback m_cb;
    CacheStatus* m_status = nullptr;
    bool m_statusOwner = false;
};

template<class K, class V, class MutexType = sylar::Mutex, class Hash = std::hash<K> >
class HashTimedLruCache {
public:
    typedef std::shared_ptr<HashTimedLruCache> ptr;
    typedef TimedLruCache<K, V, MutexType> cache_type;

    HashTimedLruCache(size_t bucket, size_t max_size, size_t elasticity)
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

    ~HashTimedLruCache() {
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
