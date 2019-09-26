#include "bitmap.h"
#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {
namespace ds {

//static uint8_t HEAD[] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};
//static uint8_t TAIL[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
//static uint8_t POS[] = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

Bitmap::base_type Bitmap::POS[sizeof(base_type) * 8];
Bitmap::base_type Bitmap::NPOS[sizeof(base_type) * 8];
Bitmap::base_type Bitmap::MASK[sizeof(base_type) * 8];

bool Bitmap::init() {
    for(size_t i = 0; i < (sizeof(base_type) * 8); ++i) {
        POS[i] = ((base_type)1) << i;
        NPOS[i] = ~POS[i];
        MASK[i] = POS[i] - 1;
    }
    return true;
}

static bool s_init = Bitmap::init();

Bitmap::Bitmap(uint32_t size, uint8_t def)
    :m_compress(false)
    ,m_size(size)
    ,m_dataSize(ceil(size * 1.0 / VALUE_SIZE))
    ,m_data(NULL) {
    if(m_dataSize) {
        m_data = (base_type*)malloc(m_dataSize * sizeof(base_type));
        memset(m_data, def, m_dataSize * sizeof(base_type));
    }
}

Bitmap::Bitmap()
    :m_compress(true)
    ,m_size(0)
    ,m_dataSize(0)
    ,m_data(NULL) {
}

Bitmap::Bitmap(const Bitmap& b) {
    m_compress = b.m_compress;
    m_size = b.m_size;
    m_dataSize = b.m_dataSize;
    m_data = (base_type*)malloc(m_dataSize * sizeof(base_type));
    memcpy(m_data, b.m_data, m_dataSize * sizeof(base_type));
}

Bitmap::~Bitmap() {
    if(m_data) {
        free(m_data);
    }
}

void Bitmap::writeTo(sylar::ByteArray::ptr ba) const {
    ba->writeFuint8(m_compress);
    ba->writeFuint32(m_size);
    ba->writeFuint32(m_dataSize);
    ba->write((const char*)m_data, m_dataSize * sizeof(base_type));
}

bool Bitmap::readFrom(sylar::ByteArray::ptr ba) {
    try {
        m_compress = ba->readFuint8();
        m_size = ba->readFuint32();
        m_dataSize = ba->readFuint32();
        if(m_data) {
            free(m_data);
        }
        m_data = (base_type*)malloc(m_dataSize * sizeof(base_type));
        ba->read((char*)m_data, m_dataSize * sizeof(base_type));
        return true;
    } catch(...) {
    }
    return false;
}

Bitmap& Bitmap::operator=(const Bitmap& b) {
    if(this == &b) {
        return *this;
    }
    m_compress = b.m_compress;
    m_size = b.m_size;
    m_dataSize = b.m_dataSize;
    if(m_data) {
        free(m_data);
    }
    m_data = (base_type*)malloc(m_dataSize * sizeof(base_type));
    memcpy(m_data, b.m_data, m_dataSize * sizeof(base_type));
    return *this;
}

Bitmap& Bitmap::operator&=(const Bitmap& b) {
    if(m_size != b.m_size) {
        throw std::logic_error("m_size != b.m_size");
    }

    if(!m_compress && !b.m_compress) {
        uint32_t max_size = m_size / U64_VALUE_SIZE;
        for(uint32_t i = 0; i < max_size; ++i) {
            ((uint64_t*)m_data)[i] &= ((uint64_t*)b.m_data)[i];
        }
        for(uint32_t i = max_size * U64_DIV_BASE;
                i < m_dataSize; ++i) {
            m_data[i] &= b.m_data[i];
        }
    } else if(m_compress != b.m_compress) {
        if(m_compress) {
            throw std::logic_error("compress &= uncompress not support");
        }
        uint32_t cur_pos = 0;
        for(uint32_t i = 0; i < b.m_dataSize; ++i) {
            base_type cur = b.m_data[i];
            if(cur & COMPRESS_MASK) {
                uint32_t count = cur & COUNT_MASK;
                bool v = cur & VALUE_MASK;
                uint32_t tmp_i = i;
                for(uint32_t n = i + 1; n < b.m_dataSize; ++n) {
                    if((b.m_data[n] & COMPRESS_MASK) && (v == (bool)(b.m_data[n] & VALUE_MASK))) {
                        count = (((uint32_t)(b.m_data[n] & COUNT_MASK)) << (VALUE_SIZE * (n - tmp_i))) | count;
                        ++i;
                    } else {
                        break;
                    }
                }
                if(!v) {
                    set(cur_pos, count, 0);
                    cur_pos += count;
                } else {
                    cur_pos += count;
                }
            } else {
                base_type count = cur & COUNT_MASK;
                m_data[cur_pos / VALUE_SIZE] &= count;
                cur_pos += VALUE_SIZE;
            }
        }
    } else {
        throw std::logic_error("compress &= compress not support");
    }
    return *this;
}

Bitmap& Bitmap::operator~() {
    if(!m_compress) {
        uint32_t max_size = m_size / U64_VALUE_SIZE;
        for(uint32_t i = 0; i < max_size; ++i) {
            ((uint64_t*)m_data)[i] = ~(((uint64_t*)m_data)[i]);
        }
        for(uint32_t i = max_size * U64_DIV_BASE;
                i < m_dataSize; ++i) {
            m_data[i] = ~(m_data[i]);
        }
        return *this;
    } else {
        for(uint32_t i = 0; i < m_dataSize; ++i) {
            base_type cur = m_data[i];
            if(cur & COMPRESS_MASK) {
                if(cur & VALUE_MASK) {
                    m_data[i] = cur & NOT_VALUE_MASK;
                } else {
                    m_data[i] = cur | VALUE_MASK;
                }
            } else {
                m_data[i] = (~cur) & COUNT_MASK;
            }
        }
        return *this;
    }
}

Bitmap& Bitmap::operator|=(const Bitmap& b) {
    if(m_size != b.m_size) {
        throw std::logic_error("m_size != b.m_size");
    }

    if(!m_compress && !b.m_compress) {
        uint32_t max_size = m_size / U64_VALUE_SIZE;
        for(uint32_t i = 0; i < max_size; ++i) {
            ((uint64_t*)m_data)[i] |= ((uint64_t*)b.m_data)[i];
        }
        for(uint32_t i = max_size * U64_DIV_BASE;
                i < m_dataSize; ++i) {
            m_data[i] |= b.m_data[i];
        }
    } else if(m_compress != b.m_compress) {
        if(m_compress) {
            throw std::logic_error("compress |= uncompress not support");
        }
        uint32_t cur_pos = 0;
        for(uint32_t i = 0; i < b.m_dataSize; ++i) {
            base_type cur = b.m_data[i];
            if(cur & COMPRESS_MASK) {
                uint32_t count = cur & COUNT_MASK;
                bool v = cur & VALUE_MASK;
                uint32_t tmp_i = i;
                for(uint32_t n = i + 1; n < b.m_dataSize; ++n) {
                    if((b.m_data[n] & COMPRESS_MASK) && (v == (bool)(b.m_data[n] & VALUE_MASK))) {
                        count = (((uint32_t)(b.m_data[n] & COUNT_MASK)) << (VALUE_SIZE * (n - tmp_i))) | count;
                        ++i;
                    } else {
                        break;
                    }
                }
                if(v) {
                    set(cur_pos, count, 1);
                    cur_pos += count;
                } else {
                    cur_pos += count;
                }
            } else {
                base_type count = cur & COUNT_MASK;
                m_data[cur_pos / VALUE_SIZE] |= count;
                cur_pos += VALUE_SIZE;
            }
        }
    } else {
        throw std::logic_error("compress |= compress not support");
    }
    return *this;
}

bool Bitmap::operator== (const Bitmap& b) const {
    if(this == &b) {
        return true;
    }
    if(m_compress != b.m_compress
            || m_size != b.m_size
            || m_dataSize != b.m_dataSize) {
        return false;
    }
    if(memcmp(m_data, b.m_data, m_dataSize * sizeof(base_type))) {
        return false;
    }
    uint32_t left = m_size % VALUE_SIZE;
    if(left) {
        //base_type mask = MASK[left];//((base_type)1 << left) - 1;
        return (m_data[m_dataSize - 1] & MASK[left])
                == (b.m_data[b.m_dataSize - 1] & MASK[left]);
    }
    return true;
}

bool Bitmap::operator!= (const Bitmap& b) const {
    return !(*this == b);
}

Bitmap Bitmap::operator& (const Bitmap& b) {
    Bitmap t(*this);
    return t &= b;
}

Bitmap Bitmap::operator| (const Bitmap& b) {
    Bitmap t(*this);
    return t |= b;
}

std::string Bitmap::toString() const {
    std::stringstream ss;
    ss << "[Bitmap compress=" << m_compress
       << " size=" << m_size
       << " data_size=" << m_dataSize
       << " data=";
    for(size_t i = 0; i < m_dataSize; ++i) {
        if(i) {
            ss << ",";
        }
        ss << m_data[i];
    }
    //if(!m_compress) {
    //    uint32_t cur_pos = 0;
    //    for(uint32_t i = 0; i < m_dataSize; ++i) {
    //        base_type cur = m_data[i];
    //        for(uint32_t n = 0; n < VALUE_SIZE && cur_pos < m_size; ++n) {
    //            ++cur_pos;
    //            ss << (bool)(cur & ((base_type)1 << n));
    //        }
    //    }
    //} else {
    //    for(uint32_t i = 0; i < m_dataSize; ++i) {
    //        if(i > 0) {
    //            ss << ",";
    //        }
    //        base_type cur = m_data[i];
    //        for(uint32_t n = 0; n < VALUE_SIZE; ++n) {
    //            ss << (bool)(cur & ((base_type)1 << n));
    //        }
    //    }
    //}
    ss << "]";
    return ss.str();
}

bool Bitmap::get(uint32_t idx) const {
    uint32_t cur = idx / VALUE_SIZE;
    uint32_t pos = idx % VALUE_SIZE;
    return m_data[cur] & POS[pos];
}

void Bitmap::set(uint32_t idx, bool v) {
    uint32_t cur = idx / VALUE_SIZE;
    uint32_t pos = idx % VALUE_SIZE;
    if(v) {
        m_data[cur] |= POS[pos];
    } else {
        m_data[cur] &= NPOS[pos];
    }
}

template<class T>
T count_bits(const T& v) {
    return __builtin_popcount(v);
}

template<>
uint64_t count_bits(const uint64_t& v) {
    return __builtin_popcount(v & 0xFFFFFFFF)
        + __builtin_popcount(v >> 32);
}

Bitmap::ptr Bitmap::compress() const{
    if(m_compress) {
        return ptr(new Bitmap(*this));
    }

    base_type* data = (base_type*)malloc(m_dataSize * sizeof(base_type));
    bool cur_value = false;
    uint64_t cur_count = 0;
    uint32_t dst_cur_pos = 0;
    for(uint32_t i = 0; i < m_dataSize; ++i) {
        base_type cur = m_data[i];
        auto c = count_bits((cur & COUNT_MASK));
        if(c == 0) {
            if(cur_value == false) {
                cur_count += VALUE_SIZE;
                continue;
            }
        } else if(c == VALUE_SIZE) {
            if(cur_value == true) {
                cur_count += VALUE_SIZE;
                continue;
            }
        }

        if((cur_count / VALUE_SIZE) > 1) {
            SYLAR_ASSERT(cur_count % VALUE_SIZE == 0);
            //SYLAR_ASSERT(cur_count < (1ul << VALUE_SIZE));
            while(cur_count) {
                data[dst_cur_pos++] = cur_value ? (COMPRESS_MASK | VALUE_MASK | (cur_count & COUNT_MASK)) : (COMPRESS_MASK | (cur_count & COUNT_MASK));
                cur_count >>= VALUE_SIZE;
            }
        } else {
            if(cur_count) {
                data[dst_cur_pos++] = cur_value ? COUNT_MASK : 0;
            }
        }

        if(c == 0 || c == VALUE_SIZE) {
            cur_value = (c == VALUE_SIZE);
            cur_count = VALUE_SIZE;
        } else {
            data[dst_cur_pos++] = cur;
            cur_count = 0;
        }
    }

    if(cur_count > 0) {
        if((cur_count / VALUE_SIZE) > 1) {
            while(cur_count) {
                data[dst_cur_pos++] = cur_value ? (COMPRESS_MASK | VALUE_MASK | (cur_count & COUNT_MASK)) : (COMPRESS_MASK | (cur_count & COUNT_MASK));
                cur_count >>= VALUE_SIZE;
            }
        } else {
            data[dst_cur_pos++] = cur_value ? (COUNT_MASK) : (0);
        }
    }

    Bitmap::ptr b(new Bitmap);
    b->m_compress = true;
    b->m_size = m_size;
    b->m_dataSize = dst_cur_pos;
    b->m_data = (base_type*)malloc(dst_cur_pos * sizeof(base_type));
    memcpy(b->m_data, data, dst_cur_pos * sizeof(base_type));
    free(data);
    return b;
}

Bitmap::ptr Bitmap::uncompress() const {
    if(!m_compress) {
        return ptr(new Bitmap(*this));
    }
    Bitmap::ptr b(new Bitmap(m_size));
    uint32_t cur_pos = 0;
    for(uint32_t i = 0; i < m_dataSize;) {
        base_type cur = m_data[i];
        if(cur & COMPRESS_MASK) {
            uint32_t count = cur & COUNT_MASK;
            bool v = cur & VALUE_MASK;
            uint32_t tmp_i = i;
            for(uint32_t n = i + 1; n < m_dataSize; ++n) {
                if((m_data[n] & COMPRESS_MASK) && (v == (bool)(m_data[n] & VALUE_MASK))) {
                    count = (((uint32_t)(m_data[n] & COUNT_MASK)) << (VALUE_SIZE * (n - tmp_i))) | count;
                    ++i;
                } else {
                    break;
                }
            }
            b->set(cur_pos, count, v);
            cur_pos += count;
            ++i;
        } else {
            base_type count = cur & COUNT_MASK;
            b->m_data[cur_pos / VALUE_SIZE] = count;
            cur_pos += VALUE_SIZE;
            ++i;
        }
    }
    return b;
}

bool Bitmap::any() const {
    if(!m_data) {
        return false;
    }
    if(!m_compress) {
        for(uint32_t i = 0; i < m_dataSize - 1; ++i) {
            if(m_data[i]) {
                return true;
            }
        }
        return m_data[m_dataSize - 1] & MASK[m_size % VALUE_SIZE]; //(((base_type)1 << (m_size % VALUE_SIZE)) - 1);
    } else {
        for(uint32_t i = 0; i < m_dataSize; ++i) {
            uint8_t cur = m_data[i];
            if(cur & COMPRESS_MASK) {
                if(cur & VALUE_MASK) {
                    return true;
                }
            } else {
                if(cur) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Bitmap::resize(uint32_t size, bool v) {
    if(m_compress) {
        throw std::logic_error("compress bitmap not support resize");
    }
    uint32_t len = ceil(size * 1.0 / VALUE_SIZE);
    if(len > m_dataSize) {
        base_type* new_data = (base_type*)malloc(len * sizeof(base_type));
        memcpy(new_data, m_data, m_dataSize * sizeof(base_type));
        if(v) {
            memset(new_data + m_dataSize, 0xFF, (len - m_dataSize) * sizeof(base_type));
        } else {
            memset(new_data + m_dataSize, 0, (len - m_dataSize) * sizeof(base_type));
        }

        if(m_data) {
            free(m_data);
        }
        m_data = new_data;

        uint32_t left = m_size % VALUE_SIZE;
        if(v) {
            new_data[m_dataSize - 1] |= MASK[left]; //((base_type)1 << left) - 1;
        } else {
            new_data[m_dataSize - 1] &= MASK[left]; //((base_type)1 << left) - 1;
        }
    }
    m_size = size;
    m_dataSize = len;
}

void Bitmap::foreach(std::function<bool(uint32_t)> cb) {
    uint32_t max_size = m_size / U64_VALUE_SIZE;
    uint32_t cur_pos = 0;
    uint64_t tmp = 0;
    for(uint32_t i = 0; i < max_size; ++i) {
        tmp = ((uint64_t*)m_data)[i];
        if(!tmp) {
            cur_pos += U64_VALUE_SIZE;
        } else {
            for(uint32_t n = 0; n < U64_DIV_BASE; ++n) {
                tmp = m_data[i * U64_DIV_BASE + n];
                for(uint32_t x = 0; x < VALUE_SIZE && cur_pos < m_size; ++x, ++cur_pos) {
                    if(tmp & POS[x]) { //((base_type)1 << x)) {
                        if(!cb(cur_pos)) {
                            return;
                        }
                    }
                }
            }
        }
    }
    for(uint32_t i = max_size * U64_DIV_BASE;
            i < m_dataSize; ++i) {
        tmp = m_data[i];
        if(!tmp) {
            cur_pos += VALUE_SIZE;
        } else {
            for(uint32_t n = 0; n < VALUE_SIZE && cur_pos < m_size; ++n, ++cur_pos) {
                if(tmp & POS[n]) { //(1UL << n)) {
                    if(!cb(cur_pos)) {
                        return;
                    }
                }
            }
        }
    }
}

void Bitmap::rforeach(std::function<bool(uint32_t)> cb) {
    uint32_t max_size = m_size / U64_VALUE_SIZE;
    uint32_t cur_pos = m_size - 1;
    uint64_t tmp = 0;

    int begin = m_dataSize - 1; 
    if(m_size % U64_VALUE_SIZE) {
        int32_t start = m_dataSize - 1;
        for(int32_t i = start; i >= (int32_t)(max_size * U64_DIV_BASE);
                --i) {
            --begin;
            tmp = m_data[i];
            if(!tmp) {
                if(i == start) {
                    if(m_size % VALUE_SIZE == 0) {
                        cur_pos -= VALUE_SIZE;
                    } else {
                        cur_pos -= m_size % VALUE_SIZE;
                    }
                } else {
                    cur_pos -= VALUE_SIZE;
                }
            } else {
                if(i == start) {
                    if(m_size % VALUE_SIZE == 0) {
                        for(int32_t n = VALUE_SIZE - 1;
                                n >= 0; --n, --cur_pos) {
                            if(tmp & POS[n]) { //((base_type)1 << n)) {
                                if(!cb(cur_pos)) {
                                    return;
                                }
                            }
                        }
                    } else {
                        cur_pos += 1;
                        for(int32_t n = m_size % VALUE_SIZE;
                                n >= 0; --n, --cur_pos) {
                            if(tmp & POS[n]) { //((base_type)1 << n)) {
                                if(!cb(cur_pos)) {
                                    return;
                                }
                            }
                        }
                    }
                } else {
                    for(int32_t n = VALUE_SIZE - 1;
                            n >= 0; --n, --cur_pos) {
                        if(tmp & POS[n]) { //((base_type)1 << n)) {
                            if(!cb(cur_pos)) {
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    //for(int32_t i = begin;
    //        i >= 0; --i){
    //    tmp = m_data[i];
    //    for(int32_t x = VALUE_SIZE - 1; x >= 0; --x, --cur_pos) {
    //        if(tmp & ((base_type)1 << x)) {
    //            if(!cb(cur_pos)) {
    //                return;
    //            }
    //        }
    //    }
    //}
    //return;

    for(int32_t i = max_size - 1; i >= 0; --i) {
        tmp = ((uint64_t*)m_data)[i];
        if(!tmp) {
            cur_pos -= U64_VALUE_SIZE;
        } else {
            for(int32_t n = 0; n < (int32_t)U64_DIV_BASE;
                    ++n) {
                tmp = m_data[(i + 1) * U64_DIV_BASE - 1 - n];
                for(int32_t x = VALUE_SIZE - 1; x >= 0; --x, --cur_pos) {
                    if(tmp & POS[x]) {//((base_type)1 << x)) {
                        if(!cb(cur_pos)) {
                            return;
                        }
                    }
                }
            }
        }
    }
}

void Bitmap::listPosAsc(std::vector<uint32_t>& pos) {
    uint32_t max_size = m_size / U64_VALUE_SIZE;
    uint32_t cur_pos = 0;
    uint64_t tmp = 0;
    for(uint32_t i = 0; i < max_size; ++i) {
        tmp = ((uint64_t*)m_data)[i];
        if(!tmp) {
            cur_pos += U64_VALUE_SIZE;
        } else {
            for(size_t x = 0; x < U64_DIV_BASE; ++x) {
                tmp = m_data[i * U64_DIV_BASE + x];
                for(uint32_t n = 0; n < VALUE_SIZE && cur_pos < m_size; ++n, ++cur_pos) {
                    if(tmp & POS[n]) { //(1UL << n)) {
                        pos.push_back(cur_pos);
                    }
                }
            }
        }
    }
    for(uint32_t i = max_size * U64_DIV_BASE; i < m_dataSize; ++i) {
        tmp = m_data[i];
        if(!tmp) {
            cur_pos += VALUE_SIZE;
        } else {
            for(uint32_t n = 0; n < VALUE_SIZE && cur_pos < m_size; ++n, ++cur_pos) {
                if(tmp & POS[n]) { //(1UL << n)) {
                    pos.push_back(cur_pos);
                }
            }
        }
    }
}

bool Bitmap::cross(const Bitmap& b) const {
    if(!m_compress && b.m_compress) {
        return compressCross(b);
    } else if(!m_compress && !b.m_compress) {
        return normalCross(b);
    } else {
        //LOG_ERROR() << "Bitmap cross invalid type";
    }
    return false;
}

bool Bitmap::compressCross(const Bitmap& b) const {
    uint32_t cur_pos = 0;
    for(uint32_t i = 0; i < b.m_dataSize; ++i) {
        base_type cur = b.m_data[i];
        if(cur & COMPRESS_MASK) {
            uint32_t count = cur & COUNT_MASK;
            bool v = cur & VALUE_MASK;
            uint32_t tmp_i = i;
            for(uint32_t n = i + 1; n < b.m_dataSize; ++n) {
                if((b.m_data[n] & COMPRESS_MASK) && (v == (bool)(b.m_data[n] & VALUE_MASK))) {
                    count = (((uint32_t)(b.m_data[n] & COUNT_MASK)) << (VALUE_SIZE * (n - tmp_i))) | count;
                    ++i;
                } else {
                    break;
                }
            }
            if(v) {
                if(get(cur_pos, count, 1)) {
                    return true;
                }
                cur_pos += count;
            } else {
                cur_pos += count;
            }
        } else {
            base_type count = cur & COUNT_MASK;
            if(m_data[cur_pos / VALUE_SIZE] & count) {
                return true;
            }
            cur_pos += VALUE_SIZE;
        }
    }
    return false;
}

bool Bitmap::normalCross(const Bitmap& b) const {
    uint32_t max_size = m_size / U64_VALUE_SIZE;
    for(uint32_t i = 0; i < max_size; ++i) {
        if((((uint64_t*)m_data)[i]) != 0) {
            if((((uint64_t*)m_data)[i]) & (((uint64_t*)b.m_data)[i])) {
                return true;
            }
        }
    }
    for(uint32_t i = max_size * U64_DIV_BASE; i < m_dataSize; ++i) {
        if(m_data[i] & b.m_data[i]) {
            return true;
        }
    }
    return false;
}

Bitmap::iterator_base::iterator_base()
    :m_pos(-1)
    ,m_size(0)
    ,m_dataSize(0)
    ,m_compress(false)
    ,m_data(NULL) {
}

bool Bitmap::iterator_base::operator!() {
    if(m_pos >= m_size || m_pos < 0) {
        return false;
    }
    return true;
}

int32_t Bitmap::iterator_base::operator*() {
    return m_pos;
}

Bitmap::iterator::iterator(Bitmap* b) {
    m_pos = -1;
    m_size = b->m_size;
    m_dataSize = b->m_dataSize;
    m_data = b->m_data;
    m_compress = b->m_compress;

    next();
}

void Bitmap::iterator::next() {
    ++m_pos;
    uint32_t pos = m_pos / VALUE_SIZE;
    uint32_t left = m_pos % VALUE_SIZE;
    base_type v = 0;
    if(left) {
        v = m_data[pos];
        for(int32_t i = left; i < (int32_t)VALUE_SIZE && m_pos < m_size; ++i, ++m_pos) {
            if(v & POS[i]) {//((base_type)1 << i)) {
                return;
            }
        }
        ++pos;
    }
    for(int32_t i = m_pos / VALUE_SIZE; i < m_dataSize && m_pos < m_size; ++i) {
        v = m_data[i];
        if(!v) {
            m_pos += VALUE_SIZE;
            continue;
        }
        for(int32_t n = 0; n < (int32_t)VALUE_SIZE && m_pos < m_size; ++n, ++m_pos) {
            if(v & POS[n]) { //((base_type)1 << n)) {
                return;
            }
        }
    }
}

Bitmap::iterator_reverse::iterator_reverse(Bitmap* b) {
    m_pos = b->m_size;
    m_size = b->m_size;
    m_dataSize = b->m_dataSize;
    m_data = b->m_data;
    m_compress = b->m_compress;

    next();
}

void Bitmap::iterator_reverse::next() {
    if(m_pos <= 0) {
        m_pos = -1;
        return;
    }

    --m_pos;
    int32_t left = (m_pos) % VALUE_SIZE;
    int32_t pos = (m_pos) / VALUE_SIZE;
    base_type v = 0;
    if(left != (VALUE_SIZE - 1)) {
        v = m_data[pos];
        for(int32_t i = left; i >= 0 && m_pos >= 0; --i, --m_pos) {
            if(v & POS[i]) { //((base_type)1 << i)) {
                return;
            }
        }
        --pos;
    }
    for(int32_t i = pos; i >= 0 && m_pos >= 0; --i) {
        v = m_data[i];
        if(!v) {
            m_pos -= VALUE_SIZE;
            continue;
        }
        for(int32_t n = VALUE_SIZE - 1; n >= 0 && m_pos >= 0; --n, --m_pos) {
            if(v & POS[n]) { //((base_type)1 << n)) {
                return;
            }
        }
    }
}

void Bitmap::set(uint32_t from, uint32_t size, bool v) {
    if(size == 0) {
        return;
    }
    uint32_t cur_from = from / VALUE_SIZE;
    uint32_t pos_from = from % VALUE_SIZE;
    uint32_t cur_to = (from + size) / VALUE_SIZE;
    uint32_t pos_to = (from + size) % VALUE_SIZE;

    SYLAR_ASSERT(pos_from == 0);
    SYLAR_ASSERT(pos_to == 0);

    for(uint32_t i = cur_from; i < cur_to; ++i) {
        m_data[i] = v ? (COUNT_MASK) : (0);
    }
}

bool Bitmap::get(uint32_t from, uint32_t size, bool v) const {
    if(size == 0) {
        return false;
    }
    uint32_t cur_from = from / VALUE_SIZE;
    uint32_t pos_from = from % VALUE_SIZE;
    uint32_t cur_to = (from + size) / VALUE_SIZE;
    uint32_t pos_to = (from + size) % VALUE_SIZE;

    SYLAR_ASSERT(pos_from == 0);
    SYLAR_ASSERT(pos_to == 0);

    for(uint32_t i = cur_from; i < cur_to; ++i) {
        if(v) {
            if(m_data[i] & COUNT_MASK) {
                return true;
            }
        } else {
            if(!(m_data[i] & COUNT_MASK)) {
                return true;
            }
        }
    }
    return false;
}

float Bitmap::getCompressRate() const {
    if(!m_compress) {
        return 100;
    } else {
        return m_dataSize * 100.0 / ceil(m_size * 1.0 / VALUE_SIZE);
    }
}

Bitmap::iterator_base::ptr Bitmap::begin_new() {
    return iterator_base::ptr(new iterator(this));
}

Bitmap::iterator_base::ptr Bitmap::rbegin_new() {
    return iterator_base::ptr(new iterator_reverse(this));
}

uint32_t Bitmap::getCount() const {
    if(!m_compress) {
        uint32_t len = m_dataSize / U64_DIV_BASE;// - (m_dataSize % 8 == 0 ? 1 : 0);
        uint32_t count = 0;
        uint32_t cur_pos = 0;
        for(uint32_t i = 0; i < len; ++i) {
            uint64_t tmp = ((uint64_t*)(m_data))[i];
            count += count_bits(tmp);
            cur_pos += U64_VALUE_SIZE;
        }
        for(uint32_t i = len * U64_DIV_BASE; i < m_dataSize; ++i) {
            base_type tmp = m_data[i];
            if(tmp) {
                for(uint32_t n = 0; n < VALUE_SIZE && cur_pos < m_size; ++n, ++cur_pos) {
                    if(tmp & POS[n]) { //(1UL << n)) {
                        ++count;
                    }
                }
            } else {
                cur_pos += VALUE_SIZE;
            }
        }
        return count;
    } else {
        uint32_t count = 0;
        for(uint32_t i = 0; i < m_dataSize; ++i) {
            base_type tmp = m_data[i];
            if(tmp & COMPRESS_MASK) {
                if(tmp & VALUE_MASK) {
                    count += tmp & COUNT_MASK;
                }
            } else {
                count += count_bits(tmp);
            }
        }
        return count;
    }
}

}
}
