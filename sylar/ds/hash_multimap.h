#ifndef __SYLAR_DS_HASH_MULTIMAP_H__
#define __SYLAR_DS_HASH_MULTIMAP_H__

#include "sylar/ds/util.h"
#include "sylar/util.h"
#include "sylar/mutex.h"
#include <memory>
#include <functional>
#include <iostream>

namespace sylar {
namespace ds {

template<class K
        ,class V
        ,class PosHash = sylar::ds::Murmur3Hash<K>
        >
class HashMultimap {
public:
    typedef std::shared_ptr<HashMultimap> ptr;
    typedef std::function<bool(const K& k, const V* v, int)> rcallback;
    typedef std::function<bool(const K& k, V* v, int)> wcallback;


    HashMultimap(const uint32_t& size = 10)
        :m_total(0)
        ,m_elements(0) {
        m_size = basket(size);
        m_datas = new std::vector<Node>[m_size]();
    }

    ~HashMultimap() {
        freeDatas(m_datas, m_size);
    }

    void rforeach(rcallback cb) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        for(size_t i = 0; i < m_size; ++i) {
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                if(!cb(n.key, n.val, n.size)) {
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
                if(!cb(n.key, n.val, n.size)) {
                    return;
                }
            }
        }
    }

    SharedArray<V> get(const K& k, bool duplicate = true) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return SharedArray<V>();
        }
        if(duplicate) {
            V* tmp = new V[it->size]();
            memcpy(tmp, it->val, sizeof(V) * it->size);
            return SharedArray<V>(it->size, tmp);
        } else {
            return SharedArray<V>(it->size, it->val, nop<V>);
        }
    }

    bool get(const K& k, std::vector<V>& v) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return SharedArray<V>();
        }
        v.resize(it->size);
        memcpy(&v[0], it->val, sizeof(V) * it->size);
        return true;
    }

    bool get(const K& k, V& v) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return false;
        }

        auto iit = BinarySearch(it->val, it->val + it->size, v);
        if(iit != it->val + it->size) {
            v = *iit;
            return true;
        }
        return false;
    }

    bool exists(const K& k) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        return it != m_datas[pos].end();
    }

    bool exists(const K& k, const V& v) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return false;
        }

        auto iit = BinarySearch(it->val, it->val + it->size, v);
        return iit != it->val + it->size;
    }

    bool insert(const K& k, const V& v) {
        if(needRehash()) {
            rehash();
        }

        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            Node node(k);
            node.size = 1;
            node.val = new V[node.size]();
            node.val[0] = v;

            m_datas[pos].push_back(node);
            SortLast(&m_datas[pos][0], m_datas[pos].size());
            //std::sort(m_datas[pos].begin(), m_datas[pos].end());

            sylar::Atomic::addFetch(m_total);
            sylar::Atomic::addFetch(m_elements);
        } else {
            auto iit = BinarySearch(it->val, it->val + it->size, v);
            if(iit != it->val + it->size) {
                *iit = v;
                return true;
            }
            V* datas = new V[it->size + 1](); 
            memcpy(datas, it->val, it->size * sizeof(V));
            datas[it->size] = v;
            ++it->size;
            SortLast(datas, it->size);
            //std::sort(datas, datas + it->size);
            if(!inValues(it->val)) {
                delete[] it->val;
            }
            it->val = datas;
            sylar::Atomic::addFetch(m_elements);
        }
        return true;
    }

    bool insert(const K& k, const V* v, const uint32_t& size) {
        if(needRehash()) {
            rehash();
        }

        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            Node node(k);
            node.size = size;
            node.val = new V[node.size]();
            memcpy(node.val, v, size * sizeof(V));
            std::sort(node.val, node.val + size);

            m_datas[pos].push_back(node);
            SortLast(&m_datas[pos][0], m_datas[pos].size());
            //std::sort(m_datas[pos].begin(), m_datas[pos].end());

            sylar::Atomic::addFetch(m_total);
            sylar::Atomic::addFetch(m_elements, (uint64_t)size);
        } else {
            std::set<V> news;
            for(size_t i = 0; i < size; ++i) {
                auto iit = BinarySearch(it->val, it->val + it->size, v[i]);
                if(iit == it->val + it->size) {
                    news.insert(v[i]);
                } else {
                    *iit = v[i];
                }
            }

            V* datas = new V[it->size + news.size()](); 
            memcpy(datas, it->val, it->size * sizeof(V));
            int n = 0;
            for(auto& i : news) {
                datas[it->size + n] = i;
                ++n;
            }
            it->size += news.size();
            std::sort(datas, datas + it->size);
            if(!inValues(it->val)) {
                delete[] it->val;
            }
            it->val = datas;
            sylar::Atomic::addFetch(m_elements, news.size());
        }
        return true;
    }

    bool set(const K& k, const V* v, const uint32_t& size) {
        if(needRehash()) {
            rehash();
        }

        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            Node node(k);
            node.size = size;
            node.val = new V[node.size]();
            memcpy(node.val, v, size * sizeof(V));
            std::sort(node.val, node.val + size);

            m_datas[pos].push_back(node);
            SortLast(&m_datas[pos][0], m_datas[pos].size());
            //std::sort(m_datas[pos].begin(), m_datas[pos].end());

            sylar::Atomic::addFetch(m_total);
            sylar::Atomic::addFetch(m_elements, (uint64_t)size);
        } else {
            V* datas = new V[size](); 
            memcpy(datas, v, size * sizeof(V));
            it->size = size;
            std::sort(datas, datas + it->size);
            if(!inValues(it->val)) {
                delete[] it->val;
            }
            it->val = datas;
            sylar::Atomic::addFetch(m_elements, (uint64_t)size);
        }
        return true;
    }


    bool del(const K& k) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return false;
        } else {
            if(!inValues(it->val)) {
                delete[] it->val;
            }
            sylar::Atomic::subFetch(m_elements, (uint64_t)it->size);
            if(m_datas[pos].size() > 1) {
                std::swap(*it, m_datas[pos].back());
            }
            m_datas[pos].resize(m_datas[pos].size() - 1);
            std::sort(m_datas[pos].begin(), m_datas[pos].end());
            sylar::Atomic::subFetch(m_total);
        }
        return true;
    }

    bool del(const K& k, const V& v) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        typename RWMutex::WriteLock lock2(s_mutex[pos % MAX_MUTEX]);

        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        if(it == m_datas[pos].end()) {
            return false;
        } else {
            auto iit = BinarySearch(it->val, it->val + it->size, v);
            if(iit == it->val + it->size) {
                return false;
            }

            if(it->size == 1) {
                if(!inValues(it->val)) {
                    delete[] it->val;
                }
                it->val = nullptr;
                it->size = 0;

                if(m_datas[pos].size() > 1) {
                    std::swap(*it, m_datas[pos].back());
                }
                m_datas[pos].resize(m_datas[pos].size() - 1);
                std::sort(m_datas[pos].begin(), m_datas[pos].end());

                sylar::Atomic::subFetch(m_total);
                sylar::Atomic::subFetch(m_elements);
                return true;
            }

            V* datas = new V[it->size - 1]();
            memcpy(datas, it->val, (iit - it->val) * sizeof(V));
            memcpy(datas, it + 1, (it->size - (iit - it->val) - 1) * sizeof(V));

            if(!inValues(it->val)) {
                delete[] it->val;
            }
            it->val = datas;
            --it->size;
        }
        return true;
    }

    void rehash() {
        sylar::RWMutex::WriteLock lock(m_mutex);
        if(needRehash()) {
            rehashUnlock();
        }
    }

    uint64_t getTotal() const { return m_total;}
    uint64_t getElements() const { return m_elements;}

    std::ostream& dump(std::ostream& os) {
        typename RWMutex::ReadLock lock(m_mutex);
        os << "[HashMultimap total=" << m_total
           << " elements=" << m_elements
           << " bucket=" << m_size
           << " rate=" << getRate()
           << "]" << std::endl;
        return os;
    }


    //for K,V is POD
    bool writeTo(std::ostream& os, uint64_t speed = -1) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        os.write((const char*)&m_size, sizeof(m_size));

        std::vector<V> vs;
        std::vector<Node> ns;

        ns.reserve(m_total);
        vs.reserve(m_total);

        for(size_t i = 0; i < m_size; ++i) {
            typename RWMutex::ReadLock lock(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                if(!n.size) {
                    continue;
                }
                size_t offset = vs.size();
                vs.insert(vs.end(), n.val, n.val + n.size);
                n.val = (V*)offset;
                ns.push_back(n);
            }
        }

        uint64_t size = vs.size() * sizeof(V);
        os.write((const char*)&size, sizeof(size));
        if(speed == (uint64_t)-1) {
            os.write((const char*)&vs[0], size);
        } else {
            sylar::WriteFixToStreamWithSpeed(os, (const char*)&vs[0], size, speed);
        }

        size = ns.size() * sizeof(Node);
        os.write((const char*)&size, sizeof(size));
        if(speed == (uint64_t)-1) {
            os.write((const char*)&ns[0], size);
        } else {
            sylar::WriteFixToStreamWithSpeed(os, (const char*)&ns[0], size, speed);
        }

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
                std::vector<V>& vs = m_values;
                std::vector<Node> ns;

                uint64_t size;
                if(!ReadFromStream(is, size)) {
                    break;
                }
                vs.resize(size / sizeof(V));
                if(speed == (uint64_t)-1) {
                    if(!ReadFixFromStream(is, (char*)&vs[0], size)) {
                        break;
                    }
                } else {
                    if(!ReadFixFromStreamWithSpeed(is, (char*)&vs[0], size, speed)) {
                        break;
                    }
                }
                if(!ReadFromStream(is, size)) {
                    break;
                }
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
                m_total = size / sizeof(Node);

                m_elements = 0;
                m_datas = new std::vector<Node>[m_size]();
                for(auto& n : ns) {
                    n.val = &vs[0] + (uint64_t)n.val;
                    m_datas[m_posHash(n.key) % m_size].push_back(n);
                    m_elements += n.size;
                }
                for(size_t i = 0; i < m_size; ++i) {
                    std::sort(m_datas[i].begin(), m_datas[i].end());
                }
                return true;
            } catch (...) {
                return false;
            }
        } while(0);
        return false;
    }
private:
    struct Node {
        K key;
        int size;
        V* val;

        Node(const K& k = K()) 
            :key(k)
            ,size(0)
            ,val(nullptr) {
        }

        bool operator<(const Node& o) const {
            return key < o.key;
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
                datas[m_posHash(n.key) % size].push_back(n);
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

    bool inValues(V* ptr) const {
        if(!ptr) {
            return true;
        }
        if(m_values.empty()) {
            return false;
        }
        return ptr >= &m_values.front() && ptr <= &m_values.back();
    }

    void freeDatas(std::vector<Node>*& datas, uint64_t size) {
        if(!datas) {
            return;
        }
        for(size_t i = 0; i < size; ++i) {
            for(auto& n : datas[i]) {
                if(n.size && !inValues(n.val)) {
                    delete [] n.val;
                }
            }
        }
        delete[] datas;
        datas = nullptr;
    }

    float getRate() const {
        return m_total * 1.0 / m_size;
    }

    bool needRehash() const {
        return getRate() > 16;
    }
private:
    uint64_t m_size;
    uint64_t m_total;
    uint64_t m_elements;
    std::vector<Node>* m_datas;
    sylar::RWMutex m_mutex;
    PosHash m_posHash;

    std::vector<V> m_values;

    static const uint32_t MAX_MUTEX = 1024 * 128;
    static sylar::RWMutex s_mutex[MAX_MUTEX];
};

template<class K
        ,class V
        ,class PosHash
        >
sylar::RWMutex HashMultimap<K, V, PosHash>::s_mutex[MAX_MUTEX];


}
}

#endif
