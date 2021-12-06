#ifndef __SYLAR_PACK_MACRO_H__
#define __SYLAR_PACK_MACRO_H__

#define PP_THIRD_ARG(a, b, c, ...) c
#define VA_OPT_SUPPORTED_I(...) PP_THIRD_ARG(__VA_OPT__(, ), 1, 0, )
#define VA_OPT_SUPPORTED VA_OPT_SUPPORTED_I(?)

// Traditional MSVC requires a special EXPAND phase
#if (defined(_MSC_VER) && !defined(_MSVC_TRADITIONAL)) ||                  \
    (defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL)

#define SYLAR_GET_ARG_COUNT(...)                                                 \
    INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define INTERNAL_EXPAND(x) x
#define INTERNAL_EXPAND_ARGS_PRIVATE(...)                                  \
    INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(                        \
        __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88,  \
        87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72,    \
        71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56,    \
        55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40,    \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24,    \
        23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7,   \
        6, 5, 4, 3, 2, 1, 0))

#else  // Other compilers

#if VA_OPT_SUPPORTED  // Standardized in C++20
#define SYLAR_GET_ARG_COUNT(...)                                                 \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                        \
        unused __VA_OPT__(, ) __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94,    \
        93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78,    \
        77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62,    \
        61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,    \
        45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30,    \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,    \
        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#elif defined(__GNUC__)  // Extension in GCC/Clang
#define SYLAR_GET_ARG_COUNT(...)                                                 \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                        \
        unused, ##__VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91,    \
        90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75,    \
        74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59,    \
        58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43,    \
        42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27,    \
        26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11,    \
        10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else  // SYLAR_GET_ARG_COUNT() may return 1 here
#define SYLAR_GET_ARG_COUNT(...)                                                 \
    INTERNAL_GET_ARG_COUNT_PRIVATE(                                        \
        unused, __VA_ARGS__, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90,  \
        89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74,    \
        73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58,    \
        57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42,    \
        41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,    \
        25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, \
        8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#endif

#define INTERNAL_GET_ARG_COUNT_PRIVATE(                                    \
    e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, e10, e11, e12, e13, e14, e15,  \
    e16, e17, e18, e19, e20, e21, e22, e23, e24, e25, e26, e27, e28, e29,  \
    e30, e31, e32, e33, e34, e35, e36, e37, e38, e39, e40, e41, e42, e43,  \
    e44, e45, e46, e47, e48, e49, e50, e51, e52, e53, e54, e55, e56, e57,  \
    e58, e59, e60, e61, e62, e63, e64, e65, e66, e67, e68, e69, e70, e71,  \
    e72, e73, e74, e75, e76, e77, e78, e79, e80, e81, e82, e83, e84, e85,  \
    e86, e87, e88, e89, e90, e91, e92, e93, e94, e95, e96, e97, e98, e99,  \
    e100, count, ...)                                                      \
    count

#define SYLAR_STR(x) #x
#define SYLAR_CONCATE(x, y) x ## y
#define _SYLAR_CONCATE(x, y) x ## y
#define SYLAR_STRING(x) SYLAR_STR(x)
#define SYLAR_PARE(...) __VA_ARGS__
#define SYLAR_EAT(...)
#define SYLAR_PAIR(x) SYLAR_PARE x // PAIR((int) x) => PARE(int) x => int x
#define SYLAR_STRIP(x) SYLAR_EAT x // STRIP((int) x) => EAT(int) x => x
#define SYLAR_PASTE(x, y) SYLAR_CONCATE(x, y)
#define SYLAR_PASTE2(x, y) _SYLAR_CONCATE(x, y)

#define SYLAR_REPEAT_0(func, i, arg)
#define SYLAR_REPEAT_1(func, i, arg)        func(i, arg)
#define SYLAR_REPEAT_2(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_1 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_3(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_2 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_4(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_3 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_5(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_4 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_6(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_5 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_7(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_6 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_8(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_7 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_9(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT_8(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_10(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_9(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_11(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_10(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_12(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_11(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_13(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_12(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_14(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_13(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_15(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_14(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_16(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_15(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_17(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_16(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_18(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_17(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_19(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_18(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_20(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_19(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_21(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_20(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_22(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_21(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_23(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_22(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_24(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_23(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_25(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_24(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_26(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_25(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_27(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_26(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_28(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_27(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_29(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_28(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_30(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_29(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_31(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_30(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_32(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_31(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_33(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_32(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_34(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_33(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_35(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_34(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_36(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_35(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_37(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_36(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_38(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_37(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_39(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_38(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_40(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_39(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_41(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_40(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_42(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_41(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_43(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_42(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_44(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_43(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_45(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_44(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_46(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_45(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_47(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_46(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_48(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_47(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_49(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_48(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_50(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_49(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_51(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_50(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_52(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_51(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_53(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_52(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_54(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_53(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_55(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_54(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_56(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_55(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_57(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_56(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_58(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_57(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_59(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_58(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_60(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_59(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_61(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_60(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_62(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_61(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_63(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_62(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_64(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT_63(func, i + 1, __VA_ARGS__)

#define SYLAR_REPEAT2_0(func, i, arg)
#define SYLAR_REPEAT2_1(func, i, arg)        func(i, arg)
#define SYLAR_REPEAT2_2(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_1 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_3(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_2 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_4(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_3 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_5(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_4 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_6(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_5 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_7(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_6 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_8(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_7 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_9(func, i, arg, ...)   func(i, arg) SYLAR_REPEAT2_8(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_10(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_9(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_11(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_10(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_12(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_11(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_13(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_12(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_14(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_13(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_15(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_14(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_16(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_15(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_17(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_16(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_18(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_17(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_19(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_18(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_20(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_19(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_21(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_20(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_22(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_21(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_23(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_22(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_24(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_23(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_25(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_24(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_26(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_25(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_27(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_26(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_28(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_27(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_29(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_28(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_30(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_29(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_31(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_30(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_32(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_31(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_33(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_32(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_34(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_33(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_35(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_34(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_36(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_35(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_37(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_36(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_38(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_37(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_39(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_38(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_40(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_39(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_41(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_40(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_42(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_41(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_43(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_42(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_44(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_43(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_45(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_44(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_46(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_45(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_47(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_46(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_48(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_47(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_49(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_48(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_50(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_49(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_51(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_50(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_52(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_51(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_53(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_52(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_54(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_53(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_55(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_54(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_56(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_55(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_57(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_56(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_58(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_57(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_59(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_58(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_60(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_59(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_61(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_60(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_62(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_61(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_63(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_62(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_64(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_63(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_65(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_64(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_66(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_65(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_67(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_66(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_68(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_67(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_69(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_68(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_70(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_69(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_71(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_70(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_72(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_71(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_73(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_72(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_74(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_73(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_75(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_74(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_76(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_75(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_77(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_76(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_78(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_77(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_79(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_78(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_80(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_79(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_81(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_80(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_82(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_81(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_83(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_82(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_84(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_83(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_85(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_84(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_86(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_85(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_87(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_86(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_88(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_87(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_89(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_88(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_90(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_89(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_91(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_90(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_92(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_91(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_93(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_92(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_94(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_93(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_95(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_94(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_96(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_95(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_97(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_96(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_98(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_97(func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT2_99(func, i, arg, ...)  func(i, arg) SYLAR_REPEAT2_98(func, i + 1, __VA_ARGS__)

#define SYLAR_REPEAT_PAIR_2(func, i, key, val)       func(i, key, val)
#define SYLAR_REPEAT_PAIR_4(func, i, key, val,...)   func(i, key, val)     SYLAR_REPEAT_PAIR_2  (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_6(func, i, key, val,...)   func(i, key, val)     SYLAR_REPEAT_PAIR_4  (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_8(func, i, key, val,...)   func(i, key, val)     SYLAR_REPEAT_PAIR_6  (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_10(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_8  (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_12(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_10 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_14(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_12 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_16(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_14 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_18(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_16 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_20(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_18 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_22(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_20 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_24(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_22 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_26(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_24 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_28(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_26 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_30(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_28 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_32(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_30 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_34(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_32 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_36(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_34 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_38(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_36 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_40(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_38 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_42(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_40 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_44(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_42 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_46(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_44 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_48(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_46 (func, i + 1, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR_50(func, i, key, val,...)  func(i, key, val)     SYLAR_REPEAT_PAIR_48 (func, i + 1, __VA_ARGS__)

#define SYLAR_REPEAT(func, i, ...) SYLAR_PASTE(SYLAR_REPEAT_, SYLAR_GET_ARG_COUNT(__VA_ARGS__)) (func, i, __VA_ARGS__)
#define SYLAR_REPEAT2(func, i, ...) SYLAR_PASTE2(SYLAR_REPEAT2_, SYLAR_GET_ARG_COUNT(__VA_ARGS__)) (func, i, __VA_ARGS__)
#define SYLAR_REPEAT_PAIR(func, i, ...) SYLAR_PASTE2(SYLAR_REPEAT_PAIR_, SYLAR_GET_ARG_COUNT(__VA_ARGS__)) (func, i, __VA_ARGS__)

#endif
