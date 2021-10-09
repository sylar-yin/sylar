#include "grpc_util.h"
#include "sylar/bytearray.h"
#include "sylar/streams/zlib_stream.h"

namespace sylar {
namespace grpc {

bool DecodeGrpcBody(const std::string& body, std::string& data, const std::string& encoding) {
    GrpcMessage tmp;
    if(!DecodeGrpcBody(body, tmp, encoding)) {
        return false;
    }
    data.swap(tmp.data);
    return true;
}

bool DecodeGrpcBody(const std::string& body, GrpcMessage& data, const std::string& encoding) {
    sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
    try {
        data.compressed = ba->readFuint8();
        data.length = ba->readFuint32();
        if(data.compressed) {
            auto zs = sylar::ZlibStream::CreateGzip(false);
            zs->write(ba, ba->getReadSize());
            zs->close();
            data.data = zs->getResult();
        } else {
            data.data = ba->toString();
        }
        return true;
    } catch (...) {
    }
    return false;
}

bool EncodeGrpcBody(const std::string& data, std::string& body, bool compress, const std::string& encoding) {
    if(compress) {
        sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
        ba->writeFuint8(1);
        auto zs = sylar::ZlibStream::CreateGzip(true);
        zs->write(data.c_str(), data.size());
        zs->close();
        auto cdata = zs->getResult();
        ba->writeFuint32(cdata.size());
        ba->write(cdata.c_str(), cdata.size());
        ba->setPosition(0);
        body = ba->toString();
    } else {
        body.resize(data.size() + 5);
        sylar::ByteArray::ptr ba(new sylar::ByteArray(&body[0], body.size()));
        ba->writeFuint8(0);
        ba->writeFuint32(data.size());
        ba->write(data.c_str(), data.size());
    }
    return true;
}

}
}
