#include "zlib_stream.h"
#include "sylar/macro.h"

namespace sylar {

ZlibStream::ptr ZlibStream::CreateGzip(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, GZIP);
}

ZlibStream::ptr ZlibStream::CreateZlib(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, ZLIB);
}

ZlibStream::ptr ZlibStream::CreateDeflate(bool encode, uint32_t buff_size) {
    return Create(encode, buff_size, DEFLATE);
}

ZlibStream::ptr ZlibStream::Create(bool encode, uint32_t buff_size,
            Type type, int level, int window_bits, int memlevel, Strategy strategy) {
    ZlibStream::ptr rt = std::make_shared<ZlibStream>(encode, buff_size);
    if(rt->init(type, level, window_bits, memlevel, strategy) == Z_OK) {
        return rt;
    }
    return nullptr;
}

ZlibStream::ZlibStream(bool encode, uint32_t buff_size)
    :m_buffSize(buff_size)
    ,m_encode(encode)
    ,m_free(true) {
}

ZlibStream::~ZlibStream() {
    if(m_free) {
        for(auto& i : m_buffs) {
            free(i.iov_base);
        }
    }

    if(m_encode) {
        deflateEnd(&m_zstream);
    } else {
        inflateEnd(&m_zstream);
    }
}

int ZlibStream::read(void* buffer, size_t length) {
    throw std::logic_error("ZlibStream::read is invalid");
}

int ZlibStream::read(ByteArray::ptr ba, size_t length) {
    throw std::logic_error("ZlibStream::read is invalid");
}

int ZlibStream::write(const void* buffer, size_t length) {
    iovec ivc;
    ivc.iov_base = (void*)buffer;
    ivc.iov_len = length;
    if(m_encode) {
        return encode(&ivc, 1, false);
    } else {
        return decode(&ivc, 1, false);
    }
}

int ZlibStream::write(ByteArray::ptr ba, size_t length) {
    std::vector<iovec> buffers;
    ba->getReadBuffers(buffers, length);
    if(m_encode) {
        return encode(&buffers[0], buffers.size(), false);
    } else {
        return decode(&buffers[0], buffers.size(), false);
    }
}

void ZlibStream::close() {
    flush();
}

int ZlibStream::init(Type type, int level, int window_bits
                    ,int memlevel, Strategy strategy) {
    SYLAR_ASSERT((level >= 0 && level <= 9) || level == DEFAULT_COMPRESSION);
    SYLAR_ASSERT((window_bits >= 8 && window_bits <= 15));
    SYLAR_ASSERT((memlevel >= 1 && memlevel <= 9));

    memset(&m_zstream, 0, sizeof(m_zstream));

    m_zstream.zalloc = Z_NULL;
    m_zstream.zfree = Z_NULL;
    m_zstream.opaque = Z_NULL;

    switch(type) {
        case DEFLATE:
            window_bits = -window_bits;
            break;
        case GZIP:
            window_bits += 16;
            break;
        case ZLIB:
        default:
            break;
    }

    if(m_encode) {
        return deflateInit2(&m_zstream, level, Z_DEFLATED
                ,window_bits, memlevel, (int)strategy);
    } else {
        return inflateInit2(&m_zstream, window_bits);
    }
}

int ZlibStream::encode(const iovec* v, const uint64_t& size, bool finish) {
    int ret = 0;
    int flush = 0;
    for(uint64_t i = 0; i < size; ++i) {
        m_zstream.avail_in = v[i].iov_len;
        m_zstream.next_in = (Bytef*)v[i].iov_base;

        flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

        iovec* ivc = nullptr;
        do {
            if(!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize) {
                ivc = &m_buffs.back();
            } else {
                iovec vc;
                vc.iov_base = malloc(m_buffSize);
                vc.iov_len = 0;
                m_buffs.push_back(vc);
                ivc = &m_buffs.back();
            }

            m_zstream.avail_out = m_buffSize - ivc->iov_len;
            m_zstream.next_out = (Bytef*)ivc->iov_base + ivc->iov_len;

            ret = deflate(&m_zstream, flush);
            if(ret == Z_STREAM_ERROR) {
                return ret;
            }
            ivc->iov_len = m_buffSize - m_zstream.avail_out;
        } while(m_zstream.avail_out == 0);
    }
    if(flush == Z_FINISH) {
        deflateEnd(&m_zstream);
    }
    return Z_OK;
}

int ZlibStream::decode(const iovec* v, const uint64_t& size, bool finish) {
    int ret = 0;
    int flush = 0;
    for(uint64_t i = 0; i < size; ++i) {
        m_zstream.avail_in = v[i].iov_len;
        m_zstream.next_in = (Bytef*)v[i].iov_base;

        flush = finish ? (i == size - 1 ? Z_FINISH : Z_NO_FLUSH) : Z_NO_FLUSH;

        iovec* ivc = nullptr;
        do {
            if(!m_buffs.empty() && m_buffs.back().iov_len != m_buffSize) {
                ivc = &m_buffs.back();
            } else {
                iovec vc;
                vc.iov_base = malloc(m_buffSize);
                vc.iov_len = 0;
                m_buffs.push_back(vc);
                ivc = &m_buffs.back();
            }

            m_zstream.avail_out = m_buffSize - ivc->iov_len;
            m_zstream.next_out = (Bytef*)ivc->iov_base + ivc->iov_len;

            ret = inflate(&m_zstream, flush);
            if(ret == Z_STREAM_ERROR) {
                return ret;
            }
            ivc->iov_len = m_buffSize - m_zstream.avail_out;
        } while(m_zstream.avail_out == 0);
    }

    if(flush == Z_FINISH) {
        inflateEnd(&m_zstream);
    }
    return Z_OK;
}

int ZlibStream::flush() {
    iovec ivc;
    ivc.iov_base = nullptr;
    ivc.iov_len = 0;

    if(m_encode) {
        return encode(&ivc, 1, true);
    } else {
        return decode(&ivc, 1, true);
    }
}

std::string ZlibStream::getResult() const {
    std::string rt;
    for(auto& i : m_buffs) {
        rt.append((const char*)i.iov_base, i.iov_len);
    }
    return rt;
}

sylar::ByteArray::ptr ZlibStream::getByteArray() {
    sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
    for(auto& i : m_buffs) {
        ba->write(i.iov_base, i.iov_len);
    }
    ba->setPosition(0);
    return ba;
}

}
