#ifndef __SYLAR_PACK_PACK_H__
#define __SYLAR_PACK_PACK_H__

#include "macro.h"

namespace sylar {
namespace pack {

struct PackFlag {
    PackFlag(uint64_t f):flag(f) {}

    enum Flag {
        
    };

    uint64_t flag;
};

template<class T>
struct is_sylar_pack_type {
    typedef char YES;
    typedef int NO;

    template<class Type, class Y>
    struct FieldMatcher;

    template<typename Type>
    static YES Test(FieldMatcher<Type, typename Type::__sylar_pack_type__>*);

    template<typename Type>
    static NO Test(...);

    static const bool value = sizeof(Test<T>(nullptr)) == sizeof(YES);
};

template<class T>
struct is_sylar_out_pack_type : public std::false_type {
};

#define SYLAR_IS_PACK(T, tt) typename std::enable_if<sylar::pack::is_sylar_pack_type<T>::value, tt>::type
#define SYLAR_NOT_PACK(T, tt) typename std::enable_if<!sylar::pack::is_sylar_pack_type<T>::value && !sylar::pack::is_sylar_out_pack_type<T>::value, tt>::type
#define SYLAR_IS_PACK_OUT(T, tt) typename std::enable_if<sylar::pack::is_sylar_out_pack_type<T>::value, tt>::type

#define _SYLAR_PACK_DECODE_OUT_O(i, name) decoder.decode(#name, v.name, flag);
#define SYLAR_PACK_DECODE_OUT_O(...) SYLAR_REPEAT2(_SYLAR_PACK_DECODE_OUT_O, 0, __VA_ARGS__)

#define _SYLAR_PACK_DECODE_OUT_A(i, attr, name) decoder.decode(attr, v.name, flag);
#define SYLAR_PACK_DECODE_OUT_A(...) SYLAR_REPEAT_PAIR(_SYLAR_PACK_DECODE_OUT_A, 0, __VA_ARGS__)

#define SYLAR_PACK_DECODE_OUT(i, arg) \
    SYLAR_PASTE(SYLAR_PACK_DECODE_OUT_, arg)

#define _SYLAR_PACK_ENCODE_OUT_O(i, name) encoder.encode(#name, v.name, flag);
#define SYLAR_PACK_ENCODE_OUT_O(...) SYLAR_REPEAT2(_SYLAR_PACK_ENCODE_OUT_O, 0, __VA_ARGS__)

#define _SYLAR_PACK_ENCODE_OUT_A(i, attr, name) encoder.encode(attr, v.name, flag);
#define SYLAR_PACK_ENCODE_OUT_A(...) SYLAR_REPEAT_PAIR(_SYLAR_PACK_ENCODE_OUT_A, 0, __VA_ARGS__)

#define SYLAR_PACK_ENCODE_OUT(i, arg) \
    SYLAR_PASTE(SYLAR_PACK_ENCODE_OUT_, arg)

#define SYLAR_PACK_OUT(C, ...) \
    namespace sylar { \
    namespace pack { \
    template<> \
    struct is_sylar_out_pack_type<C> : public std::true_type { \
    }; \
    template<class T> \
    bool __sylar_encode__(T& encoder, const C& v, const sylar::pack::PackFlag& flag) { \
        SYLAR_REPEAT(SYLAR_PACK_ENCODE_OUT, 0, __VA_ARGS__); \
        return true; \
    } \
    template<class T> \
    bool __sylar_decode__(T& decoder, C& v, const sylar::pack::PackFlag& flag) { \
        SYLAR_REPEAT(SYLAR_PACK_DECODE_OUT, 0, __VA_ARGS__); \
        return true; \
    } \
    } \
    }

#define _SYLAR_PACK_DECODE_O(i, name) decoder.decode(#name, name, flag);
#define SYLAR_PACK_DECODE_O(...) SYLAR_REPEAT2(_SYLAR_PACK_DECODE_O, 0, __VA_ARGS__)

#define _SYLAR_PACK_DECODE_A(i, attr, name) decoder.decode(attr, name, flag);
#define SYLAR_PACK_DECODE_A(...) SYLAR_REPEAT_PAIR(_SYLAR_PACK_DECODE_A, 0, __VA_ARGS__)

#define _SYLAR_PACK_DECODE_I(i, type) decoder.decode_inherit(static_cast<type&>(*this), flag);
#define SYLAR_PACK_DECODE_I(...) SYLAR_REPEAT2(_SYLAR_PACK_DECODE_I, 0, __VA_ARGS__)

#define SYLAR_PACK_DECODE(i, arg) \
    SYLAR_PASTE(SYLAR_PACK_DECODE_, arg)

#define _SYLAR_PACK_ENCODE_O(i, name) encoder.encode(#name, name, flag);
#define SYLAR_PACK_ENCODE_O(...) SYLAR_REPEAT2(_SYLAR_PACK_ENCODE_O, 0, __VA_ARGS__)

#define _SYLAR_PACK_ENCODE_A(i, attr, name) encoder.encode(attr, name, flag);
#define SYLAR_PACK_ENCODE_A(...) SYLAR_REPEAT_PAIR(_SYLAR_PACK_ENCODE_A, 0, __VA_ARGS__)

#define _SYLAR_PACK_ENCODE_I(i, type) encoder.encode_inherit(static_cast<const type&>(*this), flag);
#define SYLAR_PACK_ENCODE_I(...) SYLAR_REPEAT2(_SYLAR_PACK_ENCODE_I, 0, __VA_ARGS__)

#define SYLAR_PACK_ENCODE(i, arg) \
    SYLAR_PASTE(SYLAR_PACK_ENCODE_, arg)

#define SYLAR_PACK_FLAG_DEFINE \
    public: \
        typedef bool __sylar_pack_type__; \

#define SYLAR_PACK(...) \
    SYLAR_PACK_FLAG_DEFINE \
    template<class T> \
    bool __sylar_encode__(T& encoder, const sylar::pack::PackFlag& flag) const { \
        SYLAR_REPEAT(SYLAR_PACK_ENCODE, 0, __VA_ARGS__); \
        return true; \
    } \
    template<class T> \
    bool __sylar_decode__(T& decoder, const sylar::pack::PackFlag& flag) { \
        SYLAR_REPEAT(SYLAR_PACK_DECODE, 0, __VA_ARGS__); \
        return true; \
    } \

}
}

#endif
