#ifndef __SYLAR_GRPC_SERVLET_H__
#define __SYLAR_GRPC_SERVLET_H__

#include "sylar/http/servlet.h"
#include "sylar/grpc/grpc_stream.h"

namespace sylar {
namespace grpc {

enum class GrpcType {
    UNARY = 0,
    SERVER = 1,
    CLIENT = 2,
    BOTH = 3
};

class GrpcServlet : public sylar::http::Servlet {
public:
    typedef std::shared_ptr<GrpcServlet> ptr;
    GrpcServlet(const std::string& name, GrpcType type);
    int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::http2::Http2Session::ptr session) = 0;

    virtual int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::grpc::GrpcStreamClient::ptr stream,
                            sylar::http2::Http2Session::ptr session) = 0;

    static std::string GetGrpcPath(const std::string& ns,
                    const std::string& service, const std::string& method);

protected:
    GrpcType m_type;
};

class GrpcFunctionServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcFunctionServlet> ptr;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::http2::Http2Session::ptr session)> callback;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::grpc::GrpcStreamClient::ptr stream,
                                  sylar::http2::Http2Session::ptr session)> stream_callback;

    GrpcFunctionServlet(GrpcType type, callback cb, stream_callback scb);

    int32_t process(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResult::ptr response,
                    sylar::http2::Http2Session::ptr session) override;

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResult::ptr response,
                    sylar::grpc::GrpcStreamClient::ptr stream,
                    sylar::http2::Http2Session::ptr session) override;

    static GrpcFunctionServlet::ptr Create(GrpcType type, callback cb, stream_callback scb);
private:
    callback m_cb;
    stream_callback m_scb;
};

class GrpcServletDispatch : public sylar::http::ServletDispatch {
public:
    typedef std::shared_ptr<GrpcServletDispatch> ptr;
};

}
}

#endif
