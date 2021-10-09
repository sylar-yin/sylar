#include "grpc_protocol.h"
#include "sylar/log.h"
#include "sylar/streams/zlib_stream.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

sylar::ByteArray::ptr GrpcMessage::packData(bool gzip) const {
    sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
    if(!gzip) {
        ba->writeFuint8(0);
        ba->writeFuint32(data.size());
        ba->write(data.c_str(), data.size());
    } else {
        ba->writeFuint8(1);
        auto zs = sylar::ZlibStream::CreateGzip(true);
        zs->write(data.c_str(), data.size());
        zs->close();
        auto rt = zs->getResult();
        ba->writeFuint32(rt.size());
        ba->write(rt.c_str(), rt.size());
    }
    return ba;
}

bool GrpcRequest::setAsPB(const google::protobuf::Message& msg) {
    try {
        if(!m_data) {
            m_data = std::make_shared<GrpcMessage>();
        }
        return msg.SerializeToString(&m_data->data);
    } catch (...) {
    }
    return false;
}

std::string GrpcRequest::toString() const {
    std::stringstream ss;
    ss << "[GrpcRequest request=" << m_request
       << " data=" << m_data << "]";
    return ss.str();
}

bool GrpcResponse::setAsPB(const google::protobuf::Message& msg) {
    try {
        if(!m_data) {
            m_data = std::make_shared<GrpcMessage>();
        }
        return msg.SerializeToString(&m_data->data);
    } catch (...) {
    }
    return false;
}

std::string GrpcResponse::toString() const {
    std::stringstream ss;
    ss << "[GrpcResponse result=" << m_result
       << " error=" << m_error
       << " used=" << m_used
       << " response=" << (m_response ? m_response->toString() : "null") << "]";
    return ss.str();
}


}
}
