#ifndef __SYLAR_GRPC_GRPC_SESSION_H__
#define __SYLAR_GRPC_GRPC_SESSION_H__

#include "sylar/http2/http2_socket_stream.h"

namespace sylar {
namespace grpc {

class GrpcServer;
class GrpcSession : public http2::Http2SocketStream {
public:
    typedef std::shared_ptr<GrpcSession> ptr;
    GrpcSession(Socket::ptr sock, GrpcServer* server);
    ~GrpcSession();
protected:
    GrpcSession::ptr getSelf();
protected:
    virtual void handleRequest(http::HttpRequest::ptr req, http2::Http2Stream::ptr stream);
    AsyncSocketStream::Ctx::ptr onStreamClose(http2::Http2Stream::ptr stream) override;
    AsyncSocketStream::Ctx::ptr onHeaderEnd(http2::Http2Stream::ptr stream) override;
protected:
    GrpcServer* m_server;
};

}
}

#endif
