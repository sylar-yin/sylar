#include "sylar/ds/timed_lru_cache.h"

void test_timed_lru_cache() {
    sylar::ds::TimedLruCache<int, int> cache(30, 10);
    for(int i = 0; i < 105; ++i) {
        cache.set(i, i * 100, 1000);
    }

    for(int i = 0; i < 105; ++i) {
        int v;
        if(cache.get(i, v)) {
            std::cout << "get: " << i << " - "
                      << v
                      << " - " << cache.get(i)
                      << std::endl;
        }
    }

    cache.set(1000, 11, 1000 * 10);
    //std::cout << "expired: " << cache.expired(100, 1000 * 10) << std::endl;
    std::cout << cache.toStatusString() << std::endl;
    sleep(2);
    std::cout << "check_timeout: " << cache.checkTimeout() << std::endl;
    std::cout << cache.toStatusString() << std::endl;
}

void test_hash_timed_lru_cache() {
    sylar::ds::HashTimedLruCache<int, int> cache(2, 30, 10);
    for(int i = 0; i < 105; ++i) {
        cache.set(i, i * 100, 1000);
    }

    for(int i = 0; i < 105; ++i) {
        int v;
        if(cache.get(i, v)) {
            std::cout << "get: " << i << " - "
                      << v
                      << " - " << cache.get(i)
                      << std::endl;
        }
    }

    cache.set(1000, 11, 1000 * 10);
    //std::cout << "expired: " << cache.expired(100, 1000 * 10) << std::endl;
    std::cout << cache.toStatusString() << std::endl;
    sleep(2);
    std::cout << "check_timeout: " << cache.checkTimeout() << std::endl;
    std::cout << cache.toStatusString() << std::endl;
}

int main(int argc, char** argv) {
    test_timed_lru_cache();
    test_hash_timed_lru_cache();
    return 0;
}
