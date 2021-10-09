#ifndef __SYLAR_GRPC_GRPC_LOADBALANCE_H__
#define __SYLAR_GRPC_GRPC_LOADBALANCE_H__

#include "sylar/grpc/grpc_stream.h"
#include "sylar/grpc/grpc_protocol.h"
#include "sylar/streams/load_balance.h"

namespace sylar {
namespace grpc {

class GrpcSDLoadBalance : public SDLoadBalance {
public:
    typedef std::shared_ptr<GrpcSDLoadBalance> ptr;
    GrpcSDLoadBalance(IServiceDiscovery::ptr sd);

    virtual void start();
    virtual void stop();
    void start(const std::unordered_map<std::string
               ,std::unordered_map<std::string,std::string> >& confs);

    GrpcStream::ptr openStream(const std::string& domain, const std::string& service,
                                 sylar::http::HttpRequest::ptr request, uint64_t idx = -1);

    GrpcResponse::ptr request(const std::string& domain, const std::string& service,
                             GrpcRequest::ptr req, uint32_t timeout_ms, uint64_t idx = -1);

    GrpcResponse::ptr request(const std::string& domain, const std::string& service,
                             const std::string& method, const google::protobuf::Message& message,
                             uint32_t timeout_ms,
                             const std::map<std::string, std::string>& headers = {},
                             uint64_t idx = -1);
};


}
}

#endif
