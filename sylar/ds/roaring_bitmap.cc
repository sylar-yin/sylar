#include "roaring_bitmap.h"
#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {
namespace ds {

RoaringBitmap::RoaringBitmap() {
}

RoaringBitmap::RoaringBitmap(const RoaringBitmap& b) {
    m_bitmap = b.m_bitmap;
}

RoaringBitmap::~RoaringBitmap() {
}

void RoaringBitmap::writeTo(sylar::ByteArray::ptr ba) const {
    size_t size = m_bitmap.getSizeInBytes(false);
    ba->writeFuint32(size);
    std::vector<char> buffer(size);
    m_bitmap.write(buffer.data(), false);
    ba->write(buffer.data(), size);
}

bool RoaringBitmap::readFrom(sylar::ByteArray::ptr ba) {
    try {
        size_t size = ba->readFuint32();
        std::vector<char> buffer(size);
        ba->read(buffer.data(), size);
        m_bitmap = Roaring::read(buffer.data(), false);
        return true;
    } catch(...) {
    }
    return false;
}

RoaringBitmap& RoaringBitmap::operator=(const RoaringBitmap& b) {
    if(this == &b) {
        return *this;
    }
    m_bitmap = b.m_bitmap;
    return *this;
}

RoaringBitmap& RoaringBitmap::operator&=(const RoaringBitmap& b) {
    m_bitmap &= b.m_bitmap;
    return *this;
}

//RoaringBitmap& RoaringBitmap::operator~() {
//}

RoaringBitmap& RoaringBitmap::operator|=(const RoaringBitmap& b) {
    m_bitmap |= b.m_bitmap;
    return *this;
}

bool RoaringBitmap::operator== (const RoaringBitmap& b) const {
    if(this == &b) {
        return true;
    }
    return m_bitmap == b.m_bitmap;
}

bool RoaringBitmap::operator!= (const RoaringBitmap& b) const {
    return !(*this == b);
}

RoaringBitmap RoaringBitmap::operator& (const RoaringBitmap& b) {
    RoaringBitmap t(*this);
    return t &= b;
}

RoaringBitmap RoaringBitmap::operator| (const RoaringBitmap& b) {
    RoaringBitmap t(*this);
    return t |= b;
}

std::string RoaringBitmap::toString() const {
    std::stringstream ss;
    ss << "[RoaringBitmap count=" << getCount()
       << " size=" << m_bitmap.getSizeInBytes()
       << "]";
    return ss.str();
}

bool RoaringBitmap::get(uint32_t idx) const {
    return m_bitmap.contains(idx);
}

void RoaringBitmap::set(uint32_t idx, bool v) {
    if(v) {
        m_bitmap.add(idx);
    } else {
        m_bitmap.remove(idx);
    }
}

RoaringBitmap::ptr RoaringBitmap::compress() const{
    RoaringBitmap::ptr rt(new RoaringBitmap(*this));
    rt->m_bitmap.shrinkToFit();
    rt->m_bitmap.runOptimize();
    return rt;
}

RoaringBitmap::ptr RoaringBitmap::uncompress() const {
    RoaringBitmap::ptr rt(new RoaringBitmap(*this));
    rt->m_bitmap.removeRunCompression();
    return rt;
}

bool RoaringBitmap::any() const {
    return m_bitmap.begin() != m_bitmap.end();
}

void RoaringBitmap::foreach(std::function<bool(uint32_t)> cb) {
    for(auto it = m_bitmap.begin();
            it != m_bitmap.end(); ++it) {
        if(!cb(*it)) {
            break;
        }
    }
}

void RoaringBitmap::rforeach(std::function<bool(uint32_t)> cb) {
    for(auto it = m_bitmap.rbegin();
            it != m_bitmap.rend(); ++it) {
        if(!cb(*it)) {
            break;
        }
    }
}

void RoaringBitmap::listPosAsc(std::vector<uint32_t>& pos) {
    for(auto it = m_bitmap.begin();
            it != m_bitmap.end(); ++it) {
        pos.push_back(*it);
    }
}

bool RoaringBitmap::cross(const RoaringBitmap& b) const {
    return m_bitmap.intersect(b.m_bitmap);
}

void RoaringBitmap::set(uint32_t from, uint32_t size, bool v) {
    m_bitmap.addRange(from, from + size);
}

bool RoaringBitmap::get(uint32_t from, uint32_t size, bool v) const {
    return false;
}

float RoaringBitmap::getCompressRate() const {
    return 100;
}

uint32_t RoaringBitmap::getCount() const {
    return m_bitmap.cardinality();
}

}
}
