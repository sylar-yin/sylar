#ifndef __SYLAR_GRPC_GRPC_CONNECTION_H__
#define __SYLAR_GRPC_GRPC_CONNECTION_H__

#include "sylar/http2/http2_connection.h"
#include "grpc_protocol.h"
#include "grpc_stream.h"
#include <google/protobuf/message.h>

namespace sylar {
namespace grpc {

class GrpcConnection : public http2::Http2Connection{
public:
    typedef std::shared_ptr<GrpcConnection> ptr;
    GrpcConnection();
    ~GrpcConnection();

    GrpcResponse::ptr request(GrpcRequest::ptr req, uint64_t timeout_ms);

    GrpcResponse::ptr request(const std::string& method,
                            const google::protobuf::Message& message,
                            uint64_t timeout_ms,
                            const std::map<std::string, std::string>& headers = {});

    GrpcStream::ptr openGrpcStream(const std::string& method,
                                   const std::map<std::string, std::string>& headers = {});

    template<class Req, class Rsp>
    typename GrpcClientStreamClient<Req, Rsp>::ptr openGrpcClientStream(const std::string& method,
            const std::map<std::string, std::string>& headers = {}) {
        auto stm = openGrpcStream(method, headers);
        if(stm) {
            return std::make_shared<GrpcClientStreamClient<Req, Rsp>>(stm);
        }
        return nullptr;
    }

    template<class Req, class Rsp>
    typename GrpcClientStreamServer<Req, Rsp>::ptr openGrpcServerStream(const std::string& method,
            const google::protobuf::Message& message,
            const std::map<std::string, std::string>& headers = {}) {
        auto stm = openGrpcStream(method, headers);
        if(stm) {
            if(stm->sendMessage(message, true) > 0) {
                return std::make_shared<GrpcClientStreamServer<Req, Rsp>>(stm);
            } else {
                //TODO log
            }
        }
        return nullptr;
    }

    template<class Req, class Rsp>
    typename GrpcClientStreamBidirection<Req, Rsp>::ptr openGrpcBidirectionStream(const std::string& method,
            const std::map<std::string, std::string>& headers = {}) {
        auto stm = openGrpcStream(method, headers);
        if(stm) {
            return std::make_shared<GrpcClientStreamBidirection<Req, Rsp>>(stm);
        }
        return nullptr;
    }
};

}
}

#endif
