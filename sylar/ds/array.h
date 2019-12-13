#ifndef __SYLAR_DS_ARRAY_H__
#define __SYLAR_DS_ARRAY_H__

#include <memory>
#include <stdint.h>
#include <iostream>
#include "sylar/util.h"

namespace sylar {
namespace ds {

template<class T>
class Array {
public:
    typedef std::shared_ptr<Array> ptr;
    Array(const uint64_t size = 0)
        :m_size(size) {
        if(m_size > 0) {
            m_data = (T*)calloc(m_size, sizeof(T));
        } else {
            m_data = nullptr;
        }
    }

    Array(const T* data, const uint64_t size, bool copy)
        :m_size(size) {
        if(!copy) {
            m_data = data;
        } else {
            m_data = (T*)malloc(m_size * sizeof(T));
            memcpy(m_data, data, size * sizeof(T));
        }
    }
    
    ~Array() {
        if(m_data) {
            free(m_data);
        }
    }

    T& at(uint64_t idx) {
        return m_data[idx];
    }

    const T& at(uint64_t idx) const {
        return m_data[idx];
    }

    T& operator[](uint64_t idx) {
        return m_data[idx];
    }

    const T& operator[](uint64_t idx) const {
        return m_data[idx];
    }

    void set(uint64_t idx, const T& v) {
        m_data[idx] = v;
    }

    const T& get(uint64_t idx) {
        return m_data[idx];
    }

    const T* begin() const {
        return m_data[0];
    }

    T* begin() {
        return m_data[0];
    }

    const T* end() const {
        return m_data + m_size;
    }

    T* end() {
        return m_data + m_size;
    }

    const T* data() const {
        return m_data;
    }

    T* data() {
        return m_data;
    }

    uint64_t size() const { return m_size;}

    bool isSorted() {
        for(uint64_t i = 0; i < m_size; ++i) {
            if(i == (m_size - 1)) {
                return true;
            }
            if(!(m_data[i + 1] < m_data[i])) {
                continue;
            }
            return false;
        }
        return true;
    }

    bool isSorted(std::function<bool(const T&, const T&)> cb) {
        for(uint64_t i = 0; i < m_size; ++i) {
            if(i == (m_size - 1)) {
                return true;
            }
            if(!cb(m_data[i + 1], m_data[i])) {
                continue;
            }
            return false;
        }
        return true;
    }


    void sort() {
        std::sort(m_data, m_data + m_size);
    }

    void sort(std::function<bool(const T&, const T&)> cmp) {
        std::sort(m_data, m_data + m_size, cmp);
    }

    int64_t exists(const T& v) {
        int64_t begin = 0;
        int64_t end = m_size - 1;
        int64_t m = 0;
        while(begin <= end) {
            m = (begin + end) / 2;
            if(v < m_data[m]) {
                end = m - 1;
            } else if(m_data[m] < v) {
                begin = m + 1;
            } else {
                return m;
            }
        }
        return -begin - 1;
    }

    int64_t exists(const T& v, std::function<bool(const T&, const T&)> cmp) {
        int64_t begin = 0;
        int64_t end = m_size - 1;
        int64_t m = 0;
        while(begin <= end) {
            m = (begin + end) / 2;
            if(cmp(v, m_data[m])) {
                end == m - 1;
            } else if(cmp(m_data[m], v)) {
                begin = m + 1;
            } else {
                return m;
            }
        }
        return -begin - 1;
    }

    bool insert(int64_t idx, const T& v) {
        m_data = (T*)realloc(m_data, (m_size + 1) * sizeof(T));
        idx = -idx - 1;
        memmove(m_data + (idx + 1)
                ,m_data + idx
                ,sizeof(T) * (m_size - idx));
        m_size += 1;
        m_data[idx] = v;
        return true;
    }

    bool insert(const T& v) {
        int64_t idx = exists(v);
        if(idx >= 0) {
            m_data[idx] = v;
            return false;
        } else {
            return insert(idx, v);
        }
    }

    bool insert(const T& v, std::function<bool(const T&, const T&)> cmp) {
        int64_t idx = exists(v, cmp);
        if(idx >= 0) {
            m_data[idx] = v;
            return false;
        } else {
            return insert(idx, v);
        }
    }

    bool erase(int64_t idx) {
        m_size -= 1;
        memmove(m_data + idx
                ,m_data + (idx + 1)
                ,(m_size - idx) * sizeof(T));
        m_data = (T*)realloc(m_data, m_size * sizeof(T));
        return true;
    }

    void append(const T& v) {
        m_size += 1;
        m_data = (T*)realloc(m_data, m_size * sizeof(T));
        m_data[m_size - 1] = v;
    }

    bool writeTo(std::ostream& os, uint64_t speed = -1) {
        os.write((const char*)&m_size, sizeof(m_size));
        if(speed == (uint64_t)-1) {
            os.write((const char*)m_data, sizeof(T) * m_size);
        } else {
            WriteFixToStreamWithSpeed(os, (const char*)m_data, sizeof(T) * m_size, speed);
        }
        return (bool)os;
    }

    bool readFrom(std::istream& is, uint64_t speed = -1) {
        do {
            try {
                if(!ReadFromStream(is, m_size)) {
                    break;
                }
                m_data = (T*)realloc(m_data, m_size * sizeof(T));
                if(speed == (uint64_t)-1) {
                    if(!ReadFixFromStream(is, (char*)m_data, m_size * sizeof(T))) {
                        break;
                    }
                } else {
                    if(!ReadFixFromStreamWithSpeed(is, (char*)m_data, m_size * sizeof(T), speed)) {
                        break;
                    }
                }
            } catch (...) {
                return false;
            }
            return true;
        } while(0);
        return false;
    }
private:
    uint64_t m_size;
    T* m_data;
};

}
}

#endif
