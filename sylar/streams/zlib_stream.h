#ifndef __SYLAR_STREAMS_ZLIB_STREAM_H__
#define __SYLAR_STREAMS_ZLIB_STREAM_H__

#include "sylar/stream.h"
#include <sys/uio.h>
#include <zlib.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>

namespace sylar {

class ZlibStream : public Stream {
public:
    typedef std::shared_ptr<ZlibStream> ptr;

    enum Type {
        ZLIB,
        DEFLATE,
        GZIP
    };

    enum Strategy {
        DEFAULT = Z_DEFAULT_STRATEGY,
        FILTERED = Z_FILTERED,
        HUFFMAN = Z_HUFFMAN_ONLY,
        FIXED = Z_FIXED,
        RLE = Z_RLE
    };

    enum CompressLevel {
        NO_COMPRESSION = Z_NO_COMPRESSION,
        BEST_SPEED = Z_BEST_SPEED,
        BEST_COMPRESSION = Z_BEST_COMPRESSION,
        DEFAULT_COMPRESSION = Z_DEFAULT_COMPRESSION
    };

    static ZlibStream::ptr CreateGzip(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateZlib(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr CreateDeflate(bool encode, uint32_t buff_size = 4096);
    static ZlibStream::ptr Create(bool encode, uint32_t buff_size = 4096,
            Type type = DEFLATE, int level = DEFAULT_COMPRESSION, int window_bits = 15
            ,int memlevel = 8, Strategy strategy = DEFAULT);

    ZlibStream(bool encode, uint32_t buff_size = 4096);
    ~ZlibStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    int flush();

    bool isFree() const { return m_free;}
    void setFree(bool v) { m_free = v;}

    bool isEncode() const { return m_encode;}
    void setEndcode(bool v) { m_encode = v;}

    std::vector<iovec>& getBuffers() { return m_buffs;}
    std::string getResult() const;
    sylar::ByteArray::ptr getByteArray();
private:
    int init(Type type = DEFLATE, int level = DEFAULT_COMPRESSION
             ,int window_bits = 15, int memlevel = 8, Strategy strategy = DEFAULT);

    int encode(const iovec* v, const uint64_t& size, bool finish);
    int decode(const iovec* v, const uint64_t& size, bool finish);
private:
    z_stream m_zstream;
    uint32_t m_buffSize;
    bool m_encode;
    bool m_free;
    std::vector<iovec> m_buffs;
};

}

#endif
