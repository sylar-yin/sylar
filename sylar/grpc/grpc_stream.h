#ifndef __SYLAR_GRPC_GRPC_STREAM_H__
#define __SYLAR_GRPC_GRPC_STREAM_H__

#include "sylar/http2/http2.h"
#include "sylar/streams/load_balance.h"
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

    template<class T>
    bool setAsPB(const T& v) {
        try {
            if(!m_data) {
                m_data = std::make_shared<GrpcMessage>();
            }
            return v.SerializeToString(&m_data->data);
        } catch (...) {
        }
        return false;
    }

private:
    http::HttpRequest::ptr m_request;
    GrpcMessage::ptr m_data;
};

class GrpcResult {
public:
    typedef std::shared_ptr<GrpcResult> ptr;
    GrpcResult() {
    }
    GrpcResult(int32_t result, const std::string& err, int32_t used)
        :m_result(result), m_used(used), m_error(err) {
    }

    http::HttpResponse::ptr getResponse() const { return m_response;}
    GrpcMessage::ptr getData() const { return m_data;}
    void setResponse(http::HttpResponse::ptr v) { m_response = v;}
    void setData(GrpcMessage::ptr v) { m_data = v;}

    int getResult() const { return m_result;}
    const std::string& getError() const { return m_error;}

    void setResult(int v) { m_result = v;}
    void setError(const std::string& v) { m_error = v;}
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

    template<class T>
    bool setAsPB(const T& v) {
        try {
            if(!m_data) {
                m_data = std::make_shared<GrpcMessage>();
            }
            return v.SerializeToString(&m_data->data);
        } catch (...) {
        }
        return false;
    }

    void setUsed(int32_t v) { m_used = v;}
    int32_t getUsed() const { return m_used;}
private:
    int m_result = 0;
    int m_used = 0;
    std::string m_error;
    http::HttpResponse::ptr m_response;
    GrpcMessage::ptr m_data;
};

class GrpcConnection : public http2::Http2Connection {
public:
    typedef std::shared_ptr<GrpcConnection> ptr;
    GrpcConnection();
    GrpcResult::ptr request(GrpcRequest::ptr req, uint64_t timeout_ms);
    GrpcResult::ptr request(const std::string& method,
                              const google::protobuf::Message& message,
                              uint64_t timeout_ms,
                              const std::map<std::string, std::string>& headers = {});
};

class GrpcSDLoadBalance : public SDLoadBalance {
public:
    typedef std::shared_ptr<GrpcSDLoadBalance> ptr;
    GrpcSDLoadBalance(IServiceDiscovery::ptr sd);

    virtual void start();
    virtual void stop();
    void start(const std::unordered_map<std::string
               ,std::unordered_map<std::string,std::string> >& confs);

    GrpcResult::ptr request(const std::string& domain, const std::string& service,
                             GrpcRequest::ptr req, uint32_t timeout_ms, uint64_t idx = -1);

    GrpcResult::ptr request(const std::string& domain, const std::string& service,
                             const std::string& method, const google::protobuf::Message& message,
                             uint32_t timeout_ms,
                             const std::map<std::string, std::string>& headers = {},
                             uint64_t idx = -1);
};


}
}

#endif
