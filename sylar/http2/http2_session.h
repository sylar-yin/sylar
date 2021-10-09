#ifndef __SYLAR_HTTP2_HTTP2_SESSION_H__
#define __SYLAR_HTTP2_HTTP2_SESSION_H__

#include "http2_socket_stream.h"
#include "http2_stream.h"

namespace sylar {
namespace http2 {

class Http2Server;
class Http2Session : public Http2SocketStream {
public:
    typedef std::shared_ptr<Http2Session> ptr;
    Http2Session(Socket::ptr sock, Http2Server* server);
    ~Http2Session();
protected:
    Http2Session::ptr getSelf();
protected:
    virtual void handleRequest(http::HttpRequest::ptr req, Http2Stream::ptr stream);
    AsyncSocketStream::Ctx::ptr onStreamClose(Http2Stream::ptr stream) override;
    AsyncSocketStream::Ctx::ptr onHeaderEnd(Http2Stream::ptr stream) override;
protected:
    Http2Server* m_server;
};

}
}

#endif
