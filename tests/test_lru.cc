#include "sylar/ds/lru_cache.h"

void test_lru() {
    sylar::ds::LruCache<int, int> cache(30, 10);

    for(int i = 0; i < 105; ++i) {
        cache.set(i, i * 100);
    }

    for(int i = 0; i < 105; ++i) {
        int v;
        if(cache.get(i, v)) {
            std::cout << "get: " << i << " - " << v << std::endl;
        }
    }

    std::cout << cache.toStatusString() << std::endl;
}

void test_hash_lru() {
    sylar::ds::HashLruCache<int, int> cache(2, 30, 10);

    for(int i = 0; i < 105; ++i) {
        cache.set(i, i * 100);
    }

    for(int i = 0; i < 105; ++i) {
        int v;
        if(cache.get(i, v)) {
            std::cout << "get: " << i << " - " << v << std::endl;
        }
    }

    std::cout << cache.toStatusString() << std::endl;
}

int main(int argc, char** argv) {
    test_lru();
    test_hash_lru();
    return 0;
}
