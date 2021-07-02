#include "grpc_stream.h"
#include "sylar/log.h"
#include <sstream>

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string GrpcRequest::toString() const {
    std::stringstream ss;
    ss << "[GrpcRequest request=" << m_request
       << " data=" << m_data << "]";
    return ss.str();
}

std::string GrpcResponse::toString() const {
    std::stringstream ss;
    ss << "[GrpcResponse result=" << m_result
       << " error=" << m_error
       << " response=" << m_response << "]";
    return ss.str();
}

GrpcResponse::ptr GrpcConnection::request(GrpcRequest::ptr req, uint64_t timeout_ms) {
    auto http_req = req->getRequest();
    http_req->setHeader("content-type", "application/grpc");
    auto grpc_data = req->getData();
    std::string data;
    data.resize(grpc_data->data.size() + 5);
    sylar::ByteArray::ptr ba(new sylar::ByteArray(&data[0], data.size()));
    ba->writeFuint8(0);
    ba->writeFuint32(grpc_data->data.size());
    ba->write(grpc_data->data.c_str(), grpc_data->data.size());
    http_req->setBody(data);

    GrpcResponse::ptr rsp = std::make_shared<GrpcResponse>();
    auto result = Http2Connection::request(http_req, timeout_ms);
    SYLAR_LOG_DEBUG(g_logger) << "send data.size=" << data.size();
    rsp->setResult(result->result);
    rsp->setError(result->error);
    rsp->setResponse(result->response);
    if(!result->result) {
        rsp->setResult(sylar::TypeUtil::Atoi(result->response->getHeader("grpc-status")));
        rsp->setError(result->response->getHeader("grpc-message"));

        auto& body = result->response->getBody();
        if(!body.empty()) {
            sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
            GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
            rsp->setData(msg);

            try {
                msg->compressed = ba->readFuint8();
                msg->length = ba->readFuint32();
                msg->data = ba->toString();
            } catch (...) {
                SYLAR_LOG_ERROR(g_logger) << "invalid grpc body";
                result->result = -501;
                result->error = "invalid grpc body";
            }
        }
    }
    return rsp;
}

GrpcResponse::ptr GrpcConnection::request(const std::string& method
        ,const google::protobuf::Message& message, uint64_t timeout_ms) {
    GrpcRequest::ptr grpc_req = std::make_shared<GrpcRequest>();
    http::HttpRequest::ptr http_req = std::make_shared<http::HttpRequest>(0x20, false);
    grpc_req->setRequest(http_req);
    http_req->setMethod(http::HttpMethod::POST);
    http_req->setPath(method);
    grpc_req->setAsPB(message);

    //GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
    //msg->compressed = 0;
    //if(!message.SerializeToString(&msg->data)) {
    //    SYLAR_LOG_ERROR(g_logger) << "SerializeToString fail, " << sylar::PBToJsonString(message);
    //    GrpcResponse::ptr rsp = std::make_shared<GrpcResponse>();
    //    rsp->setResult(-100);
    //    rsp->setError("pb SerializeToString fail");
    //    return rsp;
    //}
    //msg->compressed = msg->data.size();
    //grpc_req->setData(msg);
    return request(grpc_req, timeout_ms);
}

GrpcConnection::GrpcConnection() {
    //m_autoConnect = false;
}

}
}
