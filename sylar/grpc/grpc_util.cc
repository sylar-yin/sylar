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

}
}
