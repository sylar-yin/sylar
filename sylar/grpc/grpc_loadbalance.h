#ifndef __SYLAR_GRPC_GRPC_LOADBALANCE_H__
#define __SYLAR_GRPC_GRPC_LOADBALANCE_H__

#include "grpc_connection.h"
#include "grpc_protocol.h"
#include "sylar/streams/load_balance.h"
#include "sylar/util.h"

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

    GrpcResponse::ptr request(const std::string& domain, const std::string& service,
                             GrpcRequest::ptr req, uint32_t timeout_ms, uint64_t idx = -1);

    GrpcResponse::ptr request(const std::string& domain, const std::string& service,
                             const std::string& method, PbMessagePtr message,
                             uint32_t timeout_ms,
                             const std::map<std::string, std::string>& headers = {},
                             uint64_t idx = -1);

    GrpcStream::ptr openGrpcStream(const std::string& domain, const std::string& service,
                                   const std::string& method,
                                   const std::map<std::string, std::string>& headers = {},
                                   uint64_t idx = -1);

    template<class Req, class Rsp>
    typename GrpcClientStreamServer<Req, Rsp>::ptr openGrpcServerStream(const std::string& domain,
                                   const std::string& service,
                                   const std::string& method,
                                   PbMessagePtr message,
                                   const std::map<std::string, std::string>& headers = {},
                                   uint64_t idx = -1) {
        auto conn = getConnAs<GrpcConnection>(domain, service, idx);
        if(!conn) {
            return nullptr;
        }
        return conn->openGrpcServerStream<Req, Rsp>(method, message, headers);
    }

    template<class Req, class Rsp>
    typename GrpcClientStreamClient<Req, Rsp>::ptr openGrpcClientStream(const std::string& domain,
                                   const std::string& service,
                                   const std::string& method,
                                   const std::map<std::string, std::string>& headers = {},
                                   uint64_t idx = -1) {
        auto conn = getConnAs<GrpcConnection>(domain, service, idx);
        if(!conn) {
            return nullptr;
        }
        return conn->openGrpcClientStream<Req, Rsp>(method, headers);
    }

    template<class Req, class Rsp>
    typename GrpcClientStreamBidirection<Req, Rsp>::ptr openGrpcBidirectionStream(const std::string& domain,
                                   const std::string& service,
                                   const std::string& method,
                                   const std::map<std::string, std::string>& headers = {},
                                   uint64_t idx = -1) {
        auto conn = getConnAs<GrpcConnection>(domain, service, idx);
        if(!conn) {
            return nullptr;
        }
        return conn->openGrpcBidirectionStream<Req, Rsp>(method, headers);
    }

};


}
}

#endif
