#ifndef __SYLAR_HTTP2_STREAM_H__
#define __SYLAR_HTTP2_STREAM_H__

#include "frame.h"
#include "sylar/mutex.h"
#include <unordered_map>
#include "sylar/http/http.h"
#include "sylar/ds/blocking_queue.h"
#include "hpack.h"
//#include "http2_stream.h"

namespace sylar {
namespace http2 {

/*
                                +--------+
                        send PP |        | recv PP
                       ,--------|  idle  |--------.
                      /         |        |         \
                     v          +--------+          v
              +----------+          |           +----------+
              |          |          | send H /  |          |
       ,------| reserved |          | recv H    | reserved |------.
       |      | (local)  |          |           | (remote) |      |
       |      +----------+          v           +----------+      |
       |          |             +--------+             |          |
       |          |     recv ES |        | send ES     |          |
       |   send H |     ,-------|  open  |-------.     | recv H   |
       |          |    /        |        |        \    |          |
       |          v   v         +--------+         v   v          |
       |      +----------+          |           +----------+      |
       |      |   half   |          |           |   half   |      |
       |      |  closed  |          | send R /  |  closed  |      |
       |      | (remote) |          | recv R    | (local)  |      |
       |      +----------+          |           +----------+      |
       |           |                |                 |           |
       |           | send ES /      |       recv ES / |           |
       |           | send R /       v        send R / |           |
       |           | recv R     +--------+   recv R   |           |
       | send R /  `----------->|        |<-----------'  send R / |
       | recv R                 | closed |               recv R   |
       `----------------------->|        |<----------------------'
                                +--------+

          send:   endpoint sends this frame
          recv:   endpoint receives this frame

          H:  HEADERS frame (with implied CONTINUATIONs)
          PP: PUSH_PROMISE frame (with implied CONTINUATIONs)
          ES: END_STREAM flag
          R:  RST_STREAM frame
*/

class Http2Stream;


class Stream : public std::enable_shared_from_this<Stream> {
public:
    friend class Http2Stream;
    typedef std::shared_ptr<Stream> ptr;
    typedef std::function<int32_t(Frame::ptr)> frame_handler;
    enum class State {
        IDLE                = 0x0,
        OPEN                = 0x1,
        CLOSED              = 0x2,
        RESERVED_LOCAL      = 0x3,
        RESERVED_REMOTE     = 0x4,
        HALF_CLOSE_LOCAL    = 0x5,
        HALF_CLOSE_REMOTE   = 0x6
    };
    Stream(std::shared_ptr<Http2Stream> stm, uint32_t id);
    ~Stream();

    uint32_t getId() const { return m_id;}

    static std::string StateToString(State state);

    int32_t handleFrame(Frame::ptr frame, bool is_client);
    int32_t sendFrame(Frame::ptr frame);
    int32_t sendResponse(sylar::http::HttpResponse::ptr rsp, bool end_stream = true);
    int32_t sendRequest(sylar::http::HttpRequest::ptr req, bool end_stream = true);

    std::shared_ptr<Http2Stream> getStream() const;
    State getState() const { return m_state;}

    http::HttpRequest::ptr getRequest() const { return m_request;}
    http::HttpResponse::ptr getResponse() const { return m_response;}

    int32_t updateSendWindowByDiff(int32_t diff);
    int32_t updateRecvWindowByDiff(int32_t diff);

    int32_t getSendWindow() const { return send_window;}
    int32_t getRecvWindow() const { return recv_window;}

    frame_handler getFrameHandler() const { return m_handler;}
    void setFrameHandler(frame_handler v) { m_handler = v;}

    void close();
private:
    int32_t handleHeadersFrame(Frame::ptr frame, bool is_client);
    int32_t handleDataFrame(Frame::ptr frame, bool is_client);
    int32_t handleRstStreamFrame(Frame::ptr frame, bool is_client);

    int32_t updateWindowSizeByDiff(int32_t* window_size, int32_t diff);
private:
    std::weak_ptr<Http2Stream> m_stream;
    State m_state;
    uint32_t m_id;
    http::HttpRequest::ptr m_request;
    http::HttpResponse::ptr m_response;
    HPack::ptr m_recvHPack;
    frame_handler m_handler;
    std::string m_body;

    int32_t send_window = 0;
    int32_t recv_window = 0;
};

class StreamClient : public std::enable_shared_from_this<StreamClient> {
public:
    typedef std::shared_ptr<StreamClient> ptr;

    static StreamClient::ptr Create(Stream::ptr stream);

    int32_t sendData(const std::string& data, bool end_stream = false);
    DataFrame::ptr recvData();

private:
    int32_t onFrame(Frame::ptr frame);
private:
    Stream::ptr m_stream;
    sylar::ds::BlockingQueue<DataFrame> m_data;
};

class StreamManager {
public:
    typedef std::shared_ptr<StreamManager> ptr;
    typedef sylar::RWSpinlock RWMutexType;

    Stream::ptr get(uint32_t id);
    void add(Stream::ptr stream);
    void del(uint32_t id);
    void clear();

    void foreach(std::function<void(Stream::ptr)> cb);
private:
    RWMutexType m_mutex;
    std::unordered_map<uint32_t, Stream::ptr> m_streams;
};

}
}

#endif
