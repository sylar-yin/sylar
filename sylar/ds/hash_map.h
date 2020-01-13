#ifndef __SYLAR_DS_HASH_MAP2_H__
#define __SYLAR_DS_HASH_MAP2_H__

#include "sylar/util.h"
#include "sylar/ds/util.h"
#include "sylar/mutex.h"
#include "sylar/log.h"
#include <memory>
#include <functional>
#include <iostream>

namespace sylar {
namespace ds {

template<class K
        ,class V
        ,class PosHash = sylar::ds::Murmur3Hash<K>
        >
class HashMap {
public:
    typedef std::shared_ptr<HashMap> ptr;
    typedef Pair<K, V> value_type;

    typedef std::function<bool(const K& k, const V& v)> rcallback;
    typedef std::function<bool(const K& k, V& v)> wcallback;

    HashMap(const uint32_t& size = 0)
        :m_total(0) {
        m_size = basket(size);
        m_datas = new std::vector<Node>[m_size]();
    }

    ~HashMap() {
        freeDatas(m_datas, m_size);
    }

    void rforeach(rcallback cb) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        for(size_t i = 0; i < m_size; ++i) {
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                if(!cb(n.key, n.val)) {
                    return;
                }
            }
        }
    }

    void wforeach(wcallback cb) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        for(size_t i = 0; i < m_size; ++i) {
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                if(!cb(n.key, n.val)) {
                    return;
                }
            }
        }
    }

    bool get(const K& k, V& v) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        if(it == m_datas[pos].end()) {
            return false;
        }
        v = it->val;
        return true;
    }


    bool exists(const K& k) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        return it != m_datas[pos].end();
    }

    bool set(const K& k, const V& v) {
        if(needRehash()) {
            rehash();
        }

        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        if(it == m_datas[pos].end()) {
            Node node(k);
            node.val = v;

            m_datas[pos].emplace_back(node);
            SortLast(&m_datas[pos][0], m_datas[pos].size());
            //std::sort(m_datas[pos].begin(), m_datas[pos].end());
            sylar::Atomic::addFetch(m_total);
            return true;
        } else {
            it->val = v;
            return false;
        }
    }

    uint64_t getTotal() const { return m_total;}

    bool del(const K& k) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        if(it == m_datas[pos].end()) {
            return false;
        } else {
            if(m_datas[pos].size() > 1) {
                std::swap(*it, m_datas[pos].back());
            }
            m_datas[pos].resize(m_datas[pos].size() - 1);
            std::sort(m_datas[pos].begin(), m_datas[pos].end());
            sylar::Atomic::subFetch(m_total);
        }
        return true;
    }

    void clear() {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_total = 0;
        m_size = 0;
        freeDatas(m_datas, m_size);
    }

    void swap(HashMap& oth) {
        sylar::RWMutex::WriteLock lock(m_mutex);
        sylar::RWMutex::WriteLock lock2(oth.m_mutex);
        std::swap(m_total, oth.m_total);
        std::swap(m_size, oth.m_size);
        std::swap(m_datas, oth.m_datas);
    }

    void merge(HashMap& oth) {
        std::vector<std::pair<K, V> > tmp;
        tmp.reserve(oth.getTotal());
        oth.rforeach([&tmp](const K& k, const V& v){
            tmp.emplace_back(std::make_pair(k, v));
            return true;
        });

        for(auto& i : tmp) {
            set(i.first, i.second);
        }
    }

    void rehash() {
        sylar::RWMutex::WriteLock lock(m_mutex);
        if(needRehash()) {
            rehashUnlock();
        }
    }

    std::ostream& dump(std::ostream& os) {
        typename RWMutex::ReadLock lock(m_mutex);
        os << "[HashMap total=" << m_total
           << " bucket=" << m_size
           << " rate=" << getRate()
           << "]" << std::endl;
        return os;
    }


    //for K,V is POD
    bool writeTo(std::ostream& os, uint64_t speed = -1) {
        //sylar::TimeCalc tc;
        sylar::RWMutex::ReadLock lock(m_mutex);
        os.write((const char*)&m_size, sizeof(m_size));

        std::vector<Node> ns;

        ns.reserve(m_total);
        for(size_t i = 0; i < m_size; ++i) {
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                ns.emplace_back(n);
            }
        }
        lock.unlock();
        //tc.tick("copy");

        size_t size = ns.size() * sizeof(Node);
        os.write((const char*)&size, sizeof(size));
        if(speed == (uint64_t)-1) {
            os.write((const char*)&ns[0], size);
        } else {
            sylar::WriteFixToStreamWithSpeed(os, (const char*)&ns[0], size, speed);
        }
        //tc.tick("write");
        //std::cout << "used: " << tc.elapse() / 1000.0 << "ms " << tc.toString() << std::endl;
        //std::cout << "writeTo size: " << os.tellp() << std::endl;
        return (bool)os;
    }

    bool readFrom(std::istream& is, uint64_t speed = -1) {
        do {
            try {
                freeDatas(m_datas, m_size);
                if(!ReadFromStream(is, m_size)) {
                    break;
                }
                //LOG_INFO() << "m_size: " << m_size;
                std::vector<Node> ns;

                uint64_t size;
                if(!ReadFromStream(is, size)) {
                    //LOG_ERROR() << "error";
                    break;
                }
                //LOG_INFO() << "ns_size: " << size;
                ns.resize(size / sizeof(Node));
                if(speed == (uint64_t)-1) {
                    if(!ReadFixFromStream(is, (char*)&ns[0], size)) {
                        break;
                    }
                } else {
                    if(!ReadFixFromStreamWithSpeed(is, (char*)&ns[0], size, speed)) {
                        break;
                    }
                }
                //LOG_INFO() << size << ": " << (size / sizeof(Node)) << " : " << ns.size();
                m_total = size / sizeof(Node);

                m_datas = new std::vector<Node>[m_size]();
                for(auto& n : ns) {
                    m_datas[m_posHash(n.key) % m_size].emplace_back(n);
                }
                for(size_t i = 0; i < m_size; ++i) {
                    std::sort(m_datas[i].begin(), m_datas[i].end());
                }
                return true;
            } catch (...) {
                //LOG_ERROR() << "error";
                return false;
            }
        } while(0);
        return false;
    }

    float getRate() const {
        return m_total * 1.0 / m_size;
    }
private:
    struct Node {
        K key;
        V val;

        Node(const K& k = K()) 
            :key(k)
            ,val() {
        }

        bool operator<(const Node& o) const {
            return key < o.key;
        }

        bool operator==(const Node& o) const {
            return key == o.key;
        }
    };

    void rehashUnlock() {
        uint64_t size = basket(m_total);
        if(size == m_size) {
            return;
        }
        std::vector<Node>* datas = new std::vector<Node>[size]();
        for(size_t i = 0; i < m_size; ++i) {
            for(auto& n : m_datas[i]) {
                datas[m_posHash(n.key) % size].emplace_back(n);
            }
        }
        for(size_t i = 0; i < size; ++i) {
            std::sort(datas[i].begin(), datas[i].end());
        }

        delete[] m_datas;
        //freeDatas(m_datas, m_size);

        m_size = size;
        m_datas = datas;
    }

    void freeDatas(std::vector<Node>*& datas, uint64_t size) {
        if(!datas) {
            return;
        }
        delete[] datas;
        datas = nullptr;
    }

    bool needRehash() const {
        return getRate() > 16;
    }
private:
    uint64_t m_size;
    uint64_t m_total;
    std::vector<Node>* m_datas;
    sylar::RWMutex m_mutex;
    PosHash m_posHash;

    static const uint32_t MAX_MUTEX = 1024 * 128;
    static sylar::RWMutex s_mutex[MAX_MUTEX];
};

template<class K
        ,class V
        ,class PosHash
        >
sylar::RWMutex HashMap<K, V, PosHash>::s_mutex[MAX_MUTEX];

}
}

#endif
