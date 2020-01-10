#include "sylar/sylar.h"
#include <assert.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    //SYLAR_ASSERT2(0 == 1, "abcdef xx");
}

int main(int argc, char** argv) {
    test_assert();

    int arr[] = {1,3,5,7,9,11};

    SYLAR_LOG_INFO(g_logger) << sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 0);
    SYLAR_LOG_INFO(g_logger) << sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 1);
    SYLAR_LOG_INFO(g_logger) << sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 4);
    SYLAR_LOG_INFO(g_logger) << sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 13);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 0) == -1);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 1) == 0);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 2) == -2);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 3) == 1);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 4) == -3);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 5) == 2);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 6) == -4);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 7) == 3);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 8) == -5);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 9) == 4);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 10) == -6);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 11) == 5);
    SYLAR_ASSERT(sylar::BinarySearch(arr, sizeof(arr) / sizeof(arr[0]), 12) == -7);
    return 0;
}
