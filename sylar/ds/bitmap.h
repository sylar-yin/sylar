#ifndef __SYLAR_DS_BITMAP_H__
#define __SYLAR_DS_BITMAP_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "sylar/bytearray.h"

namespace sylar {
namespace ds {

#define BITMAP_TYPE_UINT8    1
#define BITMAP_TYPE_UINT16   2 
#define BITMAP_TYPE_UINT32   3
#define BITMAP_TYPE_UINT64   4

#ifndef BITMAP_TYPE
#define BITMAP_TYPE BITMAP_TYPE_UINT16
#endif

class Bitmap {
public:
    typedef std::shared_ptr<Bitmap> ptr;
#if BITMAP_TYPE == BITMAP_TYPE_UINT8
    typedef uint8_t base_type;
#elif BITMAP_TYPE == BITMAP_TYPE_UINT16
    typedef uint16_t base_type;
#elif BITMAP_TYPE == BITMAP_TYPE_UINT32
    typedef uint32_t base_type;
#elif BITMAP_TYPE == BITMAP_TYPE_UINT64
    typedef uint64_t base_type;
#endif

    Bitmap(uint32_t size, uint8_t def = 0);
    Bitmap(const Bitmap& b);
    ~Bitmap();

    Bitmap& operator=(const Bitmap& b);

    std::string toString() const;
    bool get(uint32_t idx) const;
    void set(uint32_t idx, bool v);

    void set(uint32_t from, uint32_t size, bool v);
    bool get(uint32_t from, uint32_t size, bool v) const;

    Bitmap& operator&=(const Bitmap& b);
    Bitmap& operator|=(const Bitmap& b);

    Bitmap operator& (const Bitmap& b);
    Bitmap operator| (const Bitmap& b);

    Bitmap& operator~();

    bool operator== (const Bitmap& b) const;
    bool operator!= (const Bitmap& b) const;

    Bitmap::ptr compress() const;
    Bitmap::ptr uncompress() const;

    bool any() const;

    uint32_t getSize() const { return m_size;}
    uint32_t getDataSize() const { return m_dataSize;}
    bool isCompress() const { return m_compress;}

    void resize(uint32_t size, bool v = false);

    void listPosAsc(std::vector<uint32_t>& pos);
    //void listPosDesc(std::vector<uint32_t>& pos);

    void foreach(std::function<bool(uint32_t)> cb);
    void rforeach(std::function<bool(uint32_t)> cb);

    void writeTo(sylar::ByteArray::ptr ba) const;
    bool readFrom(sylar::ByteArray::ptr ba);

    //uncompress to compress
    //uncompress to uncompress
    bool cross(const Bitmap& b) const;

    float getCompressRate() const;

    uint32_t getCount() const;
public:
    class iterator_base {
    public:
        typedef std::shared_ptr<iterator_base> ptr;

        iterator_base();
        virtual ~iterator_base() {}

        bool operator!();
        int32_t operator*();
        virtual void next() = 0;
    protected:
        int32_t m_pos;
        int32_t m_size;
        int32_t m_dataSize;
        bool m_compress;
        base_type* m_data;
    };

    class iterator : public iterator_base {
    public:
        iterator() {}
        iterator(Bitmap* b);
        void next();
    };

    class iterator_reverse : public iterator_base {
    public:
        iterator_reverse() {}
        iterator_reverse(Bitmap* b);
        void next();
    };
public:
    iterator begin() { return iterator(this);}
    iterator_reverse rbegin() { return iterator_reverse(this);}

    typename iterator_base::ptr begin_new();
    typename iterator_base::ptr rbegin_new();
private:
    bool normalCross(const Bitmap& b) const;
    //uncompress to compress
    bool compressCross(const Bitmap& b) const;
    Bitmap();
private:
    bool m_compress;
    uint32_t m_size;
    uint32_t m_dataSize;
    base_type* m_data;
private:
    static const uint32_t VALUE_SIZE = sizeof(base_type) * 8 - 2;
    static const base_type COMPRESS_MASK = ((base_type)1 << (sizeof(base_type) * 8 - 1));
    static const base_type VALUE_MASK = ((base_type)1 << (sizeof(base_type) * 8 - 2));
    static const base_type COUNT_MASK = ((base_type)1 << (sizeof(base_type) * 8 - 2)) - 1;
    static const base_type NOT_VALUE_MASK = ~VALUE_MASK;
    static const base_type U64_DIV_BASE = (sizeof(uint64_t) / sizeof(base_type));
    static const base_type U64_VALUE_SIZE = (VALUE_SIZE * U64_DIV_BASE);

    static base_type POS[sizeof(base_type) * 8];
    static base_type NPOS[sizeof(base_type) * 8];
    static base_type MASK[sizeof(base_type) * 8];
public:
    static bool init();
};

}
}

#endif
