#ifndef __SYLAR_DS_UTIL_H__
#define __SYLAR_DS_UTIL_H__

#include "sylar/util.h"
#include <math.h>
#include <vector>

namespace sylar {
namespace ds {

template<class T, uint32_t seed = 1009150517>
class Murmur3Hash {
public:
    uint32_t operator()(const T& data) {
        return sylar::murmur3_hash(&data, sizeof(T), seed);
    }
};

template<class K, class V>
struct Pair {
    K first;
    V second;

    Pair(const K& k = K(), const V& v = V())
        :first(k)
        ,second(v) {
    }

    bool operator==(const Pair<K, V>& o) const {
        return first == o.first;
    }

    bool operator<(const Pair<K, V>& o) const {
        return first < o.first;
    }
};

template<class K, class V>
struct MultiPair {
    K first;
    V second;

    MultiPair(const K& k = K(), const V& v = V())
        :first(k)
        ,second(v) {
    }

    bool operator==(const Pair<K, V>& o) const {
        return memcmp(this, &o, sizeof(MultiPair<K, V>)) == 0;
    }

    bool operator<(const Pair<K, V>& o) const {
        return memcmp(this, &o, sizeof(MultiPair<K, V>)) < 0;
    }
};


template<class K, class V, uint32_t seed>
class Murmur3Hash<Pair<K, V>, seed> {
public:
    typedef Murmur3Hash<K, seed> KeyHashType;
    uint32_t operator()(const Pair<K, V>& data) {
        return hash(data.first);
    }

    KeyHashType hash;
};

template<uint32_t seed>
class Murmur3Hash<int32_t, seed> {
public:
    uint32_t operator()(const int& data) {
        return data;
    }
};

template<uint32_t seed>
class Murmur3Hash<uint32_t, seed> {
public:
    uint32_t operator()(const uint32_t& data) {
        return data;
    }
};

template<uint32_t seed>
class Murmur3Hash<int64_t, seed> {
public:
    uint32_t operator()(const int64_t& data) {
        return data;
    }
};

template<uint32_t seed>
class Murmur3Hash<uint64_t, seed> {
public:
    uint32_t operator()(const uint64_t& data) {
        return data;
    }
};

template<uint32_t seed>
class Murmur3Hash<std::string, seed> {
public:
    uint32_t operator()(const std::string& data) {
        return sylar::murmur3_hash(data.data(), data.size(), seed);
    }
};

template<class T, uint32_t seed>
class Murmur3Hash<std::vector<T>, seed> {
public:
    uint32_t operator()(const std::vector<T>& data) {
        return sylar::murmur3_hash(&data[0], data.size() * sizeof(T), seed);
    }
};


template<class T, uint32_t seed = 1060627423, uint32_t seed2 = 1050126127>
class Murmur3Hash64 {
public:
    uint64_t operator()(const T& data) {
        return sylar::murmur3_hash64(&data, sizeof(T), seed, seed2);
    }
};

template<uint32_t seed, uint32_t seed2>
class Murmur3Hash64<std::string, seed, seed2> {
public:
    uint64_t operator()(const std::string& data) {
        return sylar::murmur3_hash64(data.data(), data.size(), seed, seed2);
    }
};

template<class T, uint32_t seed, uint32_t seed2>
class Murmur3Hash64<std::vector<T>, seed, seed2> {
public:
    uint32_t operator()(const std::vector<T>& data) {
        return sylar::murmur3_hash64(&data[0], data.size() * sizeof(T), seed, seed2);
    }
};

class PrimeGenerator {
public:
    PrimeGenerator();
    uint32_t upperValue(uint32_t v, uint32_t skip = 0);
    uint32_t getValue();
    uint32_t nextValue();
    uint32_t prevValue();
    bool hasNext();
    bool hasNext(uint32_t dist);
    bool hasPrev();

    uint32_t getIndex() const;
    void setIndex(uint32_t idx);
private:
    const uint32_t* m_cur;
};

class RandomStringGenerator {
public:
    static std::string Gen(uint32_t size = 10);
};

template<class K, class V>
struct Ivt {
    K id;
    int count;
    V* data;

    Ivt()
        :id()
        ,count(0)
        ,data(nullptr) {
    }

    inline void convert(const V* v) {
        if(!count) {
            data = nullptr;
        } else {
            data = (V*)v + (size_t)data / sizeof(V);
        }
    }

    inline void convert(const size_t& offset) {
        if(!count) {
            data = nullptr;
        } else {
            data = (V*)offset;
        }
    }

    inline void duplicate(const std::vector<V>& v) {
        duplicate(&v[0], v.size());
    }

    inline void duplicate(const Ivt<K, V>& v) {
        id = v.id;
        duplicate(v.data, v.count);
    }

    inline void duplicate(const V* v, const int& cnt) {
        count = cnt;
        if(!count) {
            data = nullptr;
        } else {
            data = new V[count]();
            memcpy(data, v, count * sizeof(V));
        }
    }

    inline void free() {
        count = 0;
        if(!data) {
            return;
        }
        delete [] data;
        data = nullptr;
    }
};


inline int basket(const int &  n) {
    return 1 << std::min((int)(floor(log2(abs(n))) + 1), 30);
}

template<class T, class V>
T BinarySearch(const T& begin, const T& end, const V& v) {
    auto tmp = std::lower_bound(begin, end, v);
    if(tmp == end) {
        return tmp;
    }
    if(!(v < *tmp)) {
        return tmp;
    }
    return end;
}

template<class T>
void SortLast(const T& data, const int& size) {
    if(size <= 1) {
        return;
    }
    for(int i = size - 2; i >= 0; --i) {
        if(data[i + 1] < data[i]) {
            std::swap(data[i + 1], data[i]);
        } else {
            break;
        }
    }
}

}
}

#endif
