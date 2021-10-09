#ifndef __SYLAR_GRPC_GRPC_PROTOCOL_H__
#define __SYLAR_GRPC_GRPC_PROTOCOL_H__

#include <google/protobuf/message.h>
#include "sylar/bytearray.h"
#include "sylar/http/http.h"

namespace sylar {
namespace grpc {

static const std::string CONTENT_TYPE = "content-type";
static const std::string APPLICATION_GRPC_PROTO = "application/grpc+proto";
static const std::string GRPC_STATUS = "grpc-status";
static const std::string GRPC_MESSAGE = "grpc-message";
static const std::string GRPC_ACCEPT_ENCODING = "grpc-accept-encoding";
static const std::string GRPC_ENCODING = "grpc-encoding";

struct GrpcMessage {
    typedef std::shared_ptr<GrpcMessage> ptr;

    uint8_t compressed = 0;
    uint32_t length = 0;
    std::string data;

    sylar::ByteArray::ptr packData(bool gzip) const;
};

class GrpcRequest {
public:
    typedef std::shared_ptr<GrpcRequest> ptr;

    http::HttpRequest::ptr getRequest() const { return m_request;}
    GrpcMessage::ptr getData() const { return m_data;}
    void setRequest(http::HttpRequest::ptr v) { m_request = v;}
    void setData(GrpcMessage::ptr v) { m_data = v;}
    std::string toString() const;

    template<class T>
    std::shared_ptr<T> getAsPB() const {
        if(!m_data) {
            return nullptr;
        }
        try {
            std::shared_ptr<T> data = std::make_shared<T>();
            if(data->ParseFromString(m_data->data)) {
                return data;
            }
        } catch (...) {
        }
        return nullptr;
    }

    bool setAsPB(const google::protobuf::Message& msg);
private:
    http::HttpRequest::ptr m_request;
    GrpcMessage::ptr m_data;
};

class GrpcResponse {
public:
    typedef std::shared_ptr<GrpcResponse> ptr;
    GrpcResponse() {
    }

    GrpcResponse(int32_t result, const std::string& err, int32_t used)
        :m_result(result), m_used(used), m_error(err) {
    }

    int getResult() const { return m_result;}
    void setResult(int v) { m_result = v;}

    const std::string& getError() const { return m_error;}
    void setError(const std::string& v) { m_error = v;}

    void setUsed(int32_t v) { m_used = v;}
    int32_t getUsed() const { return m_used;}

    http::HttpResponse::ptr getResponse() const { return m_response;}
    void setResponse(http::HttpResponse::ptr v) { m_response = v;}

    GrpcMessage::ptr getData() const { return m_data;}
    void setData(GrpcMessage::ptr v) { m_data = v;}

    std::string toString() const;

    template<class T>
    std::shared_ptr<T> getAsPB() const {
        if(!m_data) {
            return nullptr;
        }
        try {
            std::shared_ptr<T> data = std::make_shared<T>();
            if(data->ParseFromString(m_data->data)) {
                return data;
            }
        } catch (...) {
        }
        return nullptr;
    }

    bool setAsPB(const google::protobuf::Message& msg);
private:
    int m_result = 0;
    int m_used = 0;
    std::string m_error;
    http::HttpResponse::ptr m_response;
    GrpcMessage::ptr m_data;
};

}
}

#endif
