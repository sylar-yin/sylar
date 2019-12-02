#ifndef __SYLAR_DS_DICT_H__
#define __SYLAR_DS_DICT_H__

#include "sylar/ds/util.h"
#include "sylar/util.h"
#include "sylar/mutex.h"
#include "sylar/log.h"
#include <memory>
#include <functional>
#include <iostream>

namespace sylar {
namespace ds {

class StringDict;
template<class K
        ,class V
        ,class PosHash = sylar::ds::Murmur3Hash<K>
        >
class Dict {
    friend class StringDict;
public:
    typedef std::shared_ptr<Dict> ptr;
    typedef std::function<bool(const K& k, const V* v, size_t size)> callback;

    Dict(const uint32_t& size = 0)
        :m_total(0) {
        m_size = basket(size);
        m_datas = new std::vector<Node>[m_size]();
    }

    ~Dict() {
        freeDatas(m_datas, m_size);
    }

    SharedArray<V> get(const K& k, bool duplicate = true) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
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
        sylar::RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        if(it == m_datas[pos].end()) {
            return false;
        }
        v.resize(it->size);
        memcpy(&v[0], it->val, sizeof(V) * it->size);
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

    bool insert(const K& k, const V* v, const uint32_t& size) {
        if(size == 0) {
            return true;
        }

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
            node.size = size;
            node.val = new V[node.size]();
            memcpy(node.val, v, size * sizeof(V));

            m_datas[pos].push_back(node);
            SortLast(&m_datas[pos][0], m_datas[pos].size());
            //std::sort(m_datas[pos].begin(), m_datas[pos].end());

            sylar::Atomic::addFetch(m_total);
        } else {
            if(it->size == (int)size) {
                memcpy(it->val, v, sizeof(V) * size);
            } else {
                V* datas = new V[size](); 
                memcpy(datas, v, size * sizeof(V));
                it->size = size;
                if(!inValues(it->val)) {
                    delete[] it->val;
                }
                it->val = datas;
            }
        }
        return true;
    }

    void foreach(callback cb) {
        sylar::RWMutex::ReadLock lock(m_mutex);
        for(size_t i = 0; i < m_size; ++i) {
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
                if(!cb(n.key, n.val, n.size)) {
                    break;
                }
            }
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
            if(!inValues(it->val)) {
                delete[] it->val;
            }
            if(m_datas[pos].size() > 1) {
                std::swap(*it, m_datas[pos].back());
            }
            m_datas[pos].resize(m_datas[pos].size() - 1);
            std::sort(m_datas[pos].begin(), m_datas[pos].end());
            sylar::Atomic::subFetch(m_total);
        }
        return true;
    }

    void rehash() {
        sylar::RWMutex::WriteLock lock(m_mutex);
        if(needRehash()) {
            rehashUnlock();
        }
    }

    std::ostream& dump(std::ostream& os) {
        typename RWMutex::ReadLock lock(m_mutex);
        os << "[Dict total=" << m_total
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
            sylar::RWMutex::ReadLock lock2(s_mutex[i % MAX_MUTEX]);
            for(auto n : m_datas[i]) {
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
                //LOG_INFO() << "m_size: " << m_size;
                std::vector<V>& vs = m_values;
                std::vector<Node> ns;

                uint64_t size;
                if(!ReadFromStream(is, size)) {
                    break;
                }
                //LOG_INFO() << "vs_size: " << size;
                vs.resize(size / sizeof(V));
                if(speed == (uint64_t)-1) {
                    if(!ReadFixFromStream(is, (char*)&vs[0], size)) {
                        //LOG_ERROR() << "error";
                        break;
                    }
                } else {
                    if(!ReadFixFromStreamWithSpeed(is, (char*)&vs[0], size, speed)) {
                        //LOG_ERROR() << "error";
                        break;
                    }
                }
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
                    n.val = &vs[0] + (uint64_t)n.val;
                    m_datas[m_posHash(n.key) % m_size].push_back(n);
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
private:
    std::string getString(const K& k) {
        uint32_t hashvalue = m_posHash(k);
        sylar::RWMutex::ReadLock lock(m_mutex);
        uint32_t pos = hashvalue % m_size;
        sylar::RWMutex::ReadLock lock2(s_mutex[pos % MAX_MUTEX]);
        auto it = BinarySearch(m_datas[pos].begin(), m_datas[pos].end(), Node(k));
        //SYLAR_ASSERT(it == std::find(m_datas[pos].begin(), m_datas[pos].end(), Node(k)));
        if(it == m_datas[pos].end()) {
            return std::string();
        }
        return std::string(it->val, it->size);
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
                if(!inValues(n.val)) {
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
sylar::RWMutex Dict<K, V, PosHash>::s_mutex[MAX_MUTEX];

class StringDict {
public:
    static uint64_t GetID(const std::string& str) {
        if(str.empty()) {
            return 0;
        }
        return sylar::murmur3_hash64(str.c_str(), str.size(), 1060627423, 1050126127);
    }
    static uint64_t GetID(const char* str, const uint32_t& size) {
        if(size == 0) {
            return 0;
        }
        return sylar::murmur3_hash64(str, size, 1060627423, 1050126127);
    }
    static uint64_t GetID(const char* str) {
        if(str == nullptr) {
            return 0;
        }
        return sylar::murmur3_hash64(str, 1060627423, 1050126127);
    }
    uint64_t update(const std::string& str) {
        return update(str.c_str(), str.size());
    }

    uint64_t update(const char* str, const uint32_t& size) {
        uint64_t id = GetID(str, size);
        if(id == 0) {
            return 0;
        }
        m_dict.insert(id, str, size);
        return id;
    }

    uint64_t update(const char* str) {
        uint64_t id = GetID(str);
        if(id == 0) {
            return 0;
        }
        m_dict.insert(id, str, strlen(str));
        return id;
    }

    std::string get(const uint64_t& id) {
        return m_dict.getString(id);
    }

    SharedArray<char> getRaw(const uint64_t& id, bool duplicate = true) {
        return m_dict.get(id, duplicate);
    }

    bool readFrom(std::istream& is, uint64_t speed = -1) {
        return m_dict.readFrom(is, speed);
    }

    bool writeTo(std::ostream& os, uint64_t speed = -1) {
        return m_dict.writeTo(os, speed);
    }

    std::ostream& dump(std::ostream& os) {
        return m_dict.dump(os);
    }

    void foreach(std::function<bool(const uint64_t& k, const char* v, size_t size)> cb) {
        m_dict.foreach(cb);
    }

    uint64_t getTotal() { return m_dict.getTotal();}
private:
    Dict<uint64_t, char> m_dict;
};

}
}

#endif
