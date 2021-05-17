#ifndef __SYLAR_HTTP2_HTTP2_STREAM_H__
#define __SYLAR_HTTP2_HTTP2_STREAM_H__

#include "frame.h"
#include "hpack.h"
#include "sylar/http/http.h"
#include "sylar/http/http_connection.h"
#include "sylar/streams/async_socket_stream.h"

namespace sylar {
namespace http2 {

class Http2Stream : public AsyncSocketStream {
public:
    typedef std::shared_ptr<Http2Stream> ptr;

    Http2Stream(Socket::ptr sock, uint32_t init_sn);
    ~Http2Stream();

    int32_t sendFrame(Frame::ptr frame);

    bool handleShakeClient();
    bool handleShakeServer();

    http::HttpResult::ptr request(http::HttpRequest::ptr req, uint64_t timeout_ms);
protected:
    struct FrameSendCtx : public SendCtx {
        typedef std::shared_ptr<FrameSendCtx> ptr;
        Frame::ptr frame;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;
    };

    struct RequestCtx : public Ctx {
        typedef std::shared_ptr<RequestCtx> ptr;
        http::HttpRequest::ptr request;
        http::HttpResponse::ptr response;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;
    };

    virtual Ctx::ptr doRecv() override;
private:
    DynamicTable m_sendTable;
    DynamicTable m_recvTable;
    FrameCodec::ptr m_codec;
    uint32_t m_sn;
};

}
}

#endif
