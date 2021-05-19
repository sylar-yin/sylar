#ifndef __SYLAR_GRPC_GRPC_STREAM_H__
#define __SYLAR_GRPC_GRPC_STREAM_H__

#include "sylar/http2/http2.h"
#include <google/protobuf/message.h>

namespace sylar {
namespace grpc {

struct GrpcMessage {
    typedef std::shared_ptr<GrpcMessage> ptr;

    uint8_t compressed = 0;
    uint32_t length = 0;
    std::string data;
};

class GrpcRequest {
public:
    typedef std::shared_ptr<GrpcRequest> ptr;

    http::HttpRequest::ptr getRequest() const { return m_request;}
    GrpcMessage::ptr getData() const { return m_data;}
    void setRequest(http::HttpRequest::ptr v) { m_request = v;}
    void setData(GrpcMessage::ptr v) { m_data = v;}
    std::string toString() const;
private:
    http::HttpRequest::ptr m_request;
    GrpcMessage::ptr m_data;
};

class GrpcResponse {
public:
    typedef std::shared_ptr<GrpcResponse> ptr;
    http::HttpResponse::ptr getResponse() const { return m_response;}
    GrpcMessage::ptr getData() const { return m_data;}
    void setResponse(http::HttpResponse::ptr v) { m_response = v;}
    void setData(GrpcMessage::ptr v) { m_data = v;}

    int getResult() const { return m_result;}
    const std::string& getError() const { return m_error;}

    void setResult(int v) { m_result = v;}
    void setError(const std::string& v) { m_error = v;}
    std::string toString() const;
private:
    int m_result = 0;
    std::string m_error;
    http::HttpResponse::ptr m_response;
    GrpcMessage::ptr m_data;
};

class GrpcConnection : public http2::Http2Connection {
public:
    typedef std::shared_ptr<GrpcConnection> ptr;
    GrpcResponse::ptr request(GrpcRequest::ptr req, uint64_t timeout_ms);
    GrpcResponse::ptr request(const std::string& method, const google::protobuf::Message& message, uint64_t timeout_ms);
};

}
}

#endif
