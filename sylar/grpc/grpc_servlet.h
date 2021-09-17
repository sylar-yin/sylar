#ifndef __SYLAR_GRPC_SERVLET_H__
#define __SYLAR_GRPC_SERVLET_H__

#include "sylar/http/servlet.h"
#include "sylar/grpc/grpc_stream.h"

namespace sylar {
namespace grpc {

class GrpcServlet : public sylar::http::Servlet {
public:
    typedef std::shared_ptr<GrpcServlet> ptr;
    GrpcServlet(const std::string& name);
    int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::SocketStream::ptr stream) = 0;

    static std::string GetGrpcPath(const std::string& ns,
                    const std::string& service, const std::string& method);
};

class GrpcFunctionServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcFunctionServlet> ptr;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::SocketStream::ptr stream)> callback;

    GrpcFunctionServlet(callback cb);
    int32_t process(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResult::ptr response,
                    sylar::SocketStream::ptr stream) override;

    static GrpcFunctionServlet::ptr Create(callback cb);
private:
    callback m_cb;
};

}
}

#endif
