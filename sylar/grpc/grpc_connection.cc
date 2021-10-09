#include "grpc_connection.h"
#include "sylar/log.h"
#include "grpc_util.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

GrpcConnection::GrpcConnection() {
    SYLAR_LOG_INFO(g_logger) << "GrpcConnection::GrpcConnection sock=" << m_socket << " - " << this;
}

GrpcConnection::~GrpcConnection() {
    SYLAR_LOG_INFO(g_logger) << "GrpcConnection::~GrpcConnection sock=" << m_socket << " - " << this;
}

GrpcResponse::ptr GrpcConnection::request(GrpcRequest::ptr req, uint64_t timeout_ms) {
    auto http_req = req->getRequest();
    http_req->setHeader(CONTENT_TYPE, APPLICATION_GRPC_PROTO);
    auto grpc_data = req->getData();
    std::string data;
    EncodeGrpcBody(grpc_data->data, data, false);
    http_req->setBody(data);

    GrpcResponse::ptr rsp = std::make_shared<GrpcResponse>();
    auto result = Http2Connection::request(http_req, timeout_ms);
    SYLAR_LOG_DEBUG(g_logger) << "send data.size=" << data.size();
    if(!result->result) {
        rsp->setResult(sylar::TypeUtil::Atoi(result->response->getHeader(GRPC_STATUS)));
        rsp->setError(result->response->getHeader(GRPC_MESSAGE));

        auto& body = result->response->getBody();
        if(!body.empty()) {
            GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
            if(!DecodeGrpcBody(body, *msg)) {
                result->result = -501;
                result->error = "invalid grpc body";
            }
        }

        if(result->response) {
            rsp->setResponse(result->response);
            if(!result->response->getBody().empty()) {
                GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
                if(DecodeGrpcBody(result->response->getBody(), *msg)) {
                    rsp->setData(msg);
                } else {
                    SYLAR_LOG_WARN(g_logger) << "recv grpc response fail, DecodeGrpcBody, body.size="
                        << result->response->getBody().size();
                    result->result = -501;
                    result->error = "invalid grpc body";
                }
            }
        }
    } else {
        rsp->setResult(result->result);
        rsp->setError(result->error);
    }
    return rsp;

}

GrpcResponse::ptr GrpcConnection::request(const std::string& method,
                            const google::protobuf::Message& message,
                            uint64_t timeout_ms,
                            const std::map<std::string, std::string>& headers) {
    GrpcRequest::ptr grpc_req = std::make_shared<GrpcRequest>();
    http::HttpRequest::ptr http_req = std::make_shared<http::HttpRequest>(0x20, false);
    grpc_req->setRequest(http_req);
    http_req->setMethod(http::HttpMethod::POST);
    http_req->setPath(method);
    for(auto & i : headers) {
        http_req->setHeader(i.first, i.second);
    }
    grpc_req->setAsPB(message);
    return request(grpc_req, timeout_ms);
}


GrpcStream::ptr GrpcConnection::openGrpcStream(const std::string& method,
                                               const std::map<std::string, std::string>& headers) {
    http::HttpRequest::ptr http_req = std::make_shared<http::HttpRequest>(0x20, false);
    http_req->setMethod(http::HttpMethod::POST);
    http_req->setPath(method);
    for(auto& i : headers) {
        http_req->setHeader(i.first, i.second);
    }
    http_req->setHeader(CONTENT_TYPE, APPLICATION_GRPC_PROTO);
    //http_req->setHeader("te", "trailers");
    auto stm = openStream(http_req);
    if(stm) {
        return std::make_shared<GrpcStream>(stm);
    }
    return nullptr;
}

}
}
