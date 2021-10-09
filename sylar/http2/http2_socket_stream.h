#ifndef __SYLAR_HTTP2_HTTP2_SOCKET_STREAM_H__
#define __SYLAR_HTTP2_HTTP2_SOCKET_STREAM_H__

#include "sylar/mutex.h"
#include "sylar/streams/async_socket_stream.h"
#include "frame.h"
#include "hpack.h"
#include "http2_stream.h"
#include "sylar/http/http_connection.h"
#include "http2_protocol.h"

namespace sylar {
namespace http2 {

class Http2SocketStream : public AsyncSocketStream {
public:
    friend class http2::Http2Stream;
    typedef std::shared_ptr<Http2SocketStream> ptr;
    typedef sylar::RWSpinlock RWMutexType;

    Http2SocketStream(Socket::ptr sock, bool client);
    ~Http2SocketStream();

    int32_t sendFrame(Frame::ptr frame, bool async);
    int32_t sendData(Http2Stream::ptr stream, const std::string& data, bool async, bool end_stream);

    bool handleShakeClient();
    bool handleShakeServer();

    http::HttpResult::ptr request(http::HttpRequest::ptr req, uint64_t timeout_ms);

    void handleRecvSetting(Frame::ptr frame);
    void handleSendSetting(Frame::ptr frame);

    int32_t sendGoAway(uint32_t last_stream_id, uint32_t error, const std::string& debug);
    int32_t sendSettings(const std::vector<SettingsItem>& items);
    int32_t sendSettingsAck();
    int32_t sendRstStream(uint32_t stream_id, uint32_t error_code);
    int32_t sendPing(bool ack, uint64_t v);
    int32_t sendWindowUpdate(uint32_t stream_id, uint32_t n);

    Http2Stream::ptr newStream();
    Http2Stream::ptr newStream(uint32_t id);
    Http2Stream::ptr getStream(uint32_t id);
    void delStream(uint32_t id);

    DynamicTable& getSendTable() { return m_sendTable;}
    DynamicTable& getRecvTable() { return m_recvTable;}

    Http2Settings& getOwnerSettings() { return m_owner;}
    Http2Settings& getPeerSettings() { return m_peer;}

    bool isSsl() const { return m_ssl;}

    //StreamClient::ptr openStreamClient(sylar::http::HttpRequest::ptr request);
    Http2Stream::ptr openStream(sylar::http::HttpRequest::ptr request);
    void onClose() override;
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

    struct StreamCtx : public Ctx {
        typedef std::shared_ptr<StreamCtx> ptr;
        http::HttpRequest::ptr request;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;
    };

    virtual Ctx::ptr doRecv() override;

protected:
    void handleWindowUpdate(Frame::ptr frame);
    void handleRecvData(Frame::ptr frame, Http2Stream::ptr stream);
protected:
    void updateSettings(Http2Settings& sts, SettingsFrame::ptr frame);
    //virtual void handleRequest(http::HttpRequest::ptr req, Http2Stream::ptr stream);
    void updateSendWindowByDiff(int32_t diff);
    void updateRecvWindowByDiff(int32_t diff);
    void onTimeOut(AsyncSocketStream::Ctx::ptr ctx) override;

    virtual AsyncSocketStream::Ctx::ptr onStreamClose(Http2Stream::ptr stream) = 0;
    virtual AsyncSocketStream::Ctx::ptr onHeaderEnd(Http2Stream::ptr stream) = 0;
protected:
    DynamicTable m_sendTable;
    DynamicTable m_recvTable;
    FrameCodec::ptr m_codec;
    uint32_t m_sn;
    bool m_isClient;
    bool m_ssl;
    Http2Settings m_owner;
    Http2Settings m_peer;
    Http2StreamManager m_streamMgr;

    RWMutexType m_mutex;
    std::list<Frame::ptr> m_waits;

    int32_t m_sendWindow = DEFAULT_INITIAL_WINDOW_SIZE;
    int32_t m_recvWindow = DEFAULT_INITIAL_WINDOW_SIZE;
};

}
}

#endif
