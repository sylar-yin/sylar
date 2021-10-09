#include "grpc_stream.h"
#include "grpc_protocol.h"
#include "sylar/log.h"
#include "sylar/streams/zlib_stream.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

GrpcStream::GrpcStream(http2::Http2Stream::ptr stream)
    : m_stream(stream) {
}

int32_t GrpcStream::sendData(const std::string& data, bool end_stream) {
    return m_stream->sendData(data, end_stream, true);
}

http2::DataFrame::ptr GrpcStream::recvData() {
    return m_stream->recvData();
}

int32_t GrpcStream::sendMessage(const google::protobuf::Message& msg, bool end_stream) {
    GrpcMessage m;
    msg.SerializeToString(&m.data);
    auto ba = m.packData(m_enableGzip);
    ba->setPosition(0);
    return m_stream->sendData(ba->toString(), end_stream, true);
}

std::shared_ptr<std::string> GrpcStream::recvMessageData() {
    try {
        auto df = recvData();
        if(!df) {
            return nullptr;
        }

        auto rt = std::make_shared<std::string>();
        auto& body = df->data;
        if(body.empty()) {
            return nullptr;
        }
        sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
        bool compress = ba->readFuint8();
        uint32_t length = ba->readFuint32();
        *rt += ba->toString();
        while(rt->size() < length) {
            df = recvData();
            if(!df) {
                return nullptr;
            }
            *rt += df->data;
        }
        if(compress) {
            auto zs = sylar::ZlibStream::CreateGzip(false);
            zs->write(rt->c_str(), rt->size());
            zs->close();
            *rt = zs->getResult();
        }
        return rt;
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "recvMessageData error";
    }
    return nullptr;
}

GrpcServerStream::GrpcServerStream(GrpcStream::ptr stream)
    :m_stream(stream) {
}

GrpcClientStream::GrpcClientStream(GrpcStream::ptr stream)
    :m_stream(stream) {
}

}
}
