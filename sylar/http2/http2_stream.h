#ifndef __SYLAR_HTTP2_HTTP2_STREAM_H__
#define __SYLAR_HTTP2_HTTP2_STREAM_H__

#include "frame.h"
#include "hpack.h"
#include "sylar/http/http.h"
#include "sylar/http/http_connection.h"
#include "sylar/streams/async_socket_stream.h"
#include "stream.h"

namespace sylar {
namespace http2 {

static const uint32_t DEFAULT_MAX_FRAME_SIZE = 16384;
static const uint32_t MAX_MAX_FRAME_SIZE = 0xFFFFFF;
static const uint32_t DEFAULT_HEADER_TABLE_SIZE = 4096;
static const uint32_t DEFAULT_MAX_HEADER_LIST_SIZE = 0x400000;
static const uint32_t DEFAULT_INITIAL_WINDOW_SIZE = 65535;
static const uint32_t MAX_INITIAL_WINDOW_SIZE = 0x7FFFFFFF;
static const uint32_t DEFAULT_MAX_READ_FRAME_SIZE = 1 << 20;
static const uint32_t DEFAULT_MAX_CONCURRENT_STREAMS = 0xffffffffu;

class Http2Server;

enum class Http2Error {
    OK                          = 0x0,
    PROTOCOL_ERROR              = 0x1,
    INTERNAL_ERROR              = 0x2,
    FLOW_CONTROL_ERROR          = 0x3,
    SETTINGS_TIMEOUT_ERROR      = 0x4,
    STREAM_CLOSED_ERROR         = 0x5,
    FRAME_SIZE_ERROR            = 0x6,
    REFUSED_STREAM_ERROR        = 0x7,
    CANCEL_ERROR                = 0x8,
    COMPRESSION_ERROR           = 0x9,
    CONNECT_ERROR               = 0xa,
    ENHANCE_YOUR_CALM_ERROR     = 0xb,
    INADEQUATE_SECURITY_ERROR   = 0xc,
    HTTP11_REQUIRED_ERROR       = 0xd,
};

std::string Http2ErrorToString(Http2Error error);

struct Http2Settings {
    uint32_t header_table_size = DEFAULT_HEADER_TABLE_SIZE;
    uint32_t max_header_list_size = DEFAULT_MAX_HEADER_LIST_SIZE;
    uint32_t max_concurrent_streams = DEFAULT_MAX_CONCURRENT_STREAMS;
    uint32_t max_frame_size = DEFAULT_MAX_FRAME_SIZE;
    uint32_t initial_window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    bool enable_push = 0;

   std::string toString() const;
};

class Http2Stream : public AsyncSocketStream {
public:
    friend class http2::Stream;
    typedef std::shared_ptr<Http2Stream> ptr;
    typedef sylar::RWSpinlock RWMutexType;

    Http2Stream(Socket::ptr sock, bool client);
    ~Http2Stream();

    int32_t sendFrame(Frame::ptr frame, bool async = true);
    int32_t sendData(http2::Stream::ptr stream, const std::string& data, bool async, bool end_stream = true);

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

    http2::Stream::ptr newStream();
    http2::Stream::ptr newStream(uint32_t id);
    http2::Stream::ptr getStream(uint32_t id);
    void delStream(uint32_t id);

    DynamicTable& getSendTable() { return m_sendTable;}
    DynamicTable& getRecvTable() { return m_recvTable;}

    Http2Settings& getOwnerSettings() { return m_owner;}
    Http2Settings& getPeerSettings() { return m_peer;}

    bool isSsl() const { return m_ssl;}

    StreamClient::ptr openStreamClient(sylar::http::HttpRequest::ptr request);
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
    void updateSettings(Http2Settings& sts, SettingsFrame::ptr frame);
    virtual void handleRequest(http::HttpRequest::ptr req, http2::Stream::ptr stream);
    void updateSendWindowByDiff(int32_t diff);
    void updateRecvWindowByDiff(int32_t diff);
    void onTimeOut(AsyncSocketStream::Ctx::ptr ctx) override;
protected:
    DynamicTable m_sendTable;
    DynamicTable m_recvTable;
    FrameCodec::ptr m_codec;
    uint32_t m_sn;
    bool m_isClient;
    bool m_ssl;
    Http2Settings m_owner;
    Http2Settings m_peer;
    StreamManager m_streamMgr;
    Http2Server* m_server;

    RWMutexType m_mutex;
    std::list<Frame::ptr> m_waits;

    int32_t send_window = DEFAULT_INITIAL_WINDOW_SIZE;
    int32_t recv_window = DEFAULT_INITIAL_WINDOW_SIZE;
};

class Http2Session : public Http2Stream {
public:
    typedef std::shared_ptr<Http2Session> ptr;
    Http2Session(Socket::ptr sock, Http2Server* server);
};

class Http2Connection : public Http2Stream {
public:
    typedef std::shared_ptr<Http2Connection> ptr;
    Http2Connection();
    bool connect(sylar::Address::ptr addr, bool ssl = false);
    void reset();
};

void Http2InitRequestForWrite(sylar::http::HttpRequest::ptr req, bool ssl = false);
void Http2InitResponseForWrite(sylar::http::HttpResponse::ptr rsp);

void Http2InitRequestForRead(sylar::http::HttpRequest::ptr req);
void Http2InitResponseForRead(sylar::http::HttpResponse::ptr rsp);

}
}

#endif
