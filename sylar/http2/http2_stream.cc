#include "http2_stream.h"
#include "sylar/log.h"
#include "http2_server.h"

namespace sylar {
namespace http2 {

static const std::string CLIENT_PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static const std::vector<std::string> s_http2error_strings = {
    "OK",
    "PROTOCOL_ERROR",
    "INTERNAL_ERROR",
    "FLOW_CONTROL_ERROR",
    "SETTINGS_TIMEOUT_ERROR",
    "STREAM_CLOSED_ERROR",
    "FRAME_SIZE_ERROR",
    "REFUSED_STREAM_ERROR",
    "CANCEL_ERROR",
    "COMPRESSION_ERROR",
    "CONNECT_ERROR",
    "ENHANCE_YOUR_CALM_ERROR",
    "INADEQUATE_SECURITY_ERROR",
    "HTTP11_REQUIRED_ERROR",
};

std::string Http2ErrorToString(Http2Error error) {
    static uint32_t SIZE = s_http2error_strings.size();
    uint8_t v = (uint8_t)error;
    if(v < SIZE) {
        return s_http2error_strings[v];
    }
    return "UNKNOW(" + std::to_string((uint32_t)v) + ")";
}

std::string Http2Settings::toString() const {
    std::stringstream ss;
    ss << "[Http2Settings header_table_size=" << header_table_size
       << " max_header_list_size=" << max_header_list_size
       << " max_concurrent_streams=" << max_concurrent_streams
       << " max_frame_size=" << max_frame_size
       << " initial_window_size=" << initial_window_size
       << " enable_push=" << enable_push << "]";
    return ss.str();
}

Http2Stream::Http2Stream(Socket::ptr sock, bool client)
    :AsyncSocketStream(sock, true)
    ,m_sn(client ? -1 : 0)
    ,m_isClient(client)
    ,m_ssl(false) {
    m_codec = std::make_shared<FrameCodec>();
    m_server = nullptr;
}

Http2Stream::~Http2Stream() {
    SYLAR_LOG_INFO(g_logger) << "Http2Stream::~Http2Stream " << this;
}

bool Http2Stream::handleShakeClient() {
    SYLAR_LOG_INFO(g_logger) << "handleShakeClient";
    if(!isConnected()) {
        return false;
    }

    int rt = writeFixSize(CLIENT_PREFACE.c_str(), CLIENT_PREFACE.size());
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeClient CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno);
        return false;
    }

    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    auto sf = std::make_shared<SettingsFrame>();
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::ENABLE_PUSH, 0);
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::INITIAL_WINDOW_SIZE, 4194304);
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::MAX_HEADER_LIST_SIZE, 10485760);
    frame->data = sf;

    rt = m_codec->serializeTo(shared_from_this(), frame);
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeClient Settings fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno);
        return false;
    }
    sendWindowUpdate(0, 1 << 30);
    return true;
}

bool Http2Stream::handleShakeServer() {
    ByteArray::ptr ba = std::make_shared<ByteArray>();
    int rt = readFixSize(ba, CLIENT_PREFACE.size());
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno);
        return false;
    }
    ba->setPosition(0);
    if(ba->toString() != CLIENT_PREFACE) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno)
            << " hex: " << ba->toHexString();
        return false;
    }
    auto frame = m_codec->parseFrom(shared_from_this());
    if(!frame) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv SettingsFrame fail,"
            << " errno=" << errno << " - " << strerror(errno);
        return false;
    }
    if(frame->header.type != (uint8_t)FrameType::SETTINGS) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv Frame not SettingsFrame, type="
            << FrameTypeToString((FrameType)frame->header.type);
        return false;
    }
    handleRecvSetting(frame);
    sendSettingsAck();
    sendSettings({});
    return true;
}

int32_t Http2Stream::sendFrame(Frame::ptr frame) {
    if(isConnected()) {
        FrameSendCtx::ptr ctx = std::make_shared<FrameSendCtx>();
        ctx->frame = frame;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}

AsyncSocketStream::Ctx::ptr Http2Stream::doRecv() {
    auto frame = m_codec->parseFrom(shared_from_this());
    if(!frame) {
        innerClose();
        return nullptr;
    }
    SYLAR_LOG_DEBUG(g_logger) << "recv: " << frame->toString();
    if(frame->header.identifier) {
        auto stream = getStream(frame->header.identifier);
        if(!stream) {
            if(m_isClient) {
                SYLAR_LOG_ERROR(g_logger) << "doRecv stream id="
                    << frame->header.identifier << " not exists "
                    << frame->toString();
                return nullptr;
            } else {
                stream = newStream(frame->header.identifier);
                if(!stream) {
                    sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                    return nullptr;
                }
            }
        }

        stream->handleFrame(frame, m_isClient);
        if(frame->header.type == (uint8_t)FrameType::DATA) {
            if(frame->header.length) {
                static uint32_t cc_up = 0;
                cc_up += frame->header.length;
                sendWindowUpdate(frame->header.identifier, frame->header.length);
                SYLAR_LOG_DEBUG(g_logger) << "update size=" << cc_up;
                //std::vector<SettingsItem> items;
                //items.emplace_back((uint8_t)SettingsFrame::Settings::ENABLE_PUSH, 0);
                //items.emplace_back((uint8_t)SettingsFrame::Settings::INITIAL_WINDOW_SIZE, 4194304);
                //items.emplace_back((uint8_t)SettingsFrame::Settings::MAX_HEADER_LIST_SIZE, 10485760);
                //sendSettings(items);
            }
        }
        if(stream->getState() == http2::Stream::State::CLOSED) {
            if(m_isClient) {
                delStream(stream->getId());
                RequestCtx::ptr ctx = getAndDelCtxAs<RequestCtx>(stream->getId());
                if(!ctx) {
                    SYLAR_LOG_WARN(g_logger) << "Http2Stream request timeout response";
                    return nullptr;
                }
                ctx->response = stream->getResponse();
                return ctx;
            } else {
                auto req = stream->getRequest();
                if(!req) {
                    SYLAR_LOG_DEBUG(g_logger) << "Http2Stream recv http request fail, errno="
                        << errno << " errstr=" << strerror(errno);
                    sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                    delStream(stream->getId());
                    return nullptr;
                }
                m_worker->schedule(std::bind(&Http2Stream::handleRequest, this, req, stream));
            }
        }
    } else {
        if(frame->header.type == (uint8_t)FrameType::SETTINGS) {
            if(!(frame->header.flags & (uint8_t)FrameFlagSettings::ACK)) {
                handleRecvSetting(frame);
                sendSettingsAck();
            }
        } else if(frame->header.type == (uint8_t)FrameType::PING) {
            if(!(frame->header.flags & (uint8_t)FrameFlagPing::ACK)) {
                auto data = std::dynamic_pointer_cast<PingFrame>(frame->data);
                sendPing(true, data->uint64);
            }
        }
    }
    return nullptr;
}

void Http2Stream::handleRequest(http::HttpRequest::ptr req, http2::Stream::ptr stream) {
    http::HttpResponse::ptr rsp = std::make_shared<http::HttpResponse>(req->getVersion(), false);
    SYLAR_LOG_DEBUG(g_logger) << *req;
    rsp->setHeader("server", m_server->getName());
    m_server->getServletDispatch()->handle(req, rsp, nullptr);
    stream->sendResponse(rsp);
    delStream(stream->getId());
}

bool Http2Stream::FrameSendCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<Http2Stream>(stream)
                ->m_codec->serializeTo(stream, frame) > 0;
}

bool Http2Stream::RequestCtx::doSend(AsyncSocketStream::ptr stream) {
    auto h2stream = std::dynamic_pointer_cast<Http2Stream>(stream);
    //auto stm = h2stream.getStream(sn);
    //if(!stm) {
    //    SYLAR_LOG_ERROR(g_logger) << "RequestCtx doSend Fail, sn=" << sn
    //        << " not exists";
    //    return false;
    //}
    Frame::ptr headers = std::make_shared<Frame>();
    headers->header.type = (uint8_t)FrameType::HEADERS;
    headers->header.flags = (uint8_t)FrameFlagHeaders::END_HEADERS;
    if(request->getBody().empty()) {
        headers->header.flags |= (uint8_t)FrameFlagHeaders::END_STREAM;
    }
    headers->header.identifier = sn;
    HeadersFrame::ptr data;
    auto m = request->getHeaders();
    data = std::make_shared<HeadersFrame>();

    HPack hp(h2stream->m_sendTable);
    std::vector<std::pair<std::string, std::string> > hs;
    for(auto& i : m) {
        hs.push_back(std::make_pair(sylar::ToLower(i.first), i.second));
    }
    hs.push_back(std::make_pair("stream_id", std::to_string(sn)));
    hp.pack(hs, data->data);
    headers->data = data;
    bool ok = std::dynamic_pointer_cast<Http2Stream>(stream)
                ->m_codec->serializeTo(stream, headers) > 0;
    if(!ok) {
        SYLAR_LOG_INFO(g_logger) << "sendHeaders fail";
        return ok;
    }
    if(!request->getBody().empty()) {
        Frame::ptr body = std::make_shared<Frame>();
        body->header.type = (uint8_t)FrameType::DATA;
        body->header.flags = (uint8_t)FrameFlagData::END_STREAM;
        body->header.identifier = sn;
        auto data = std::make_shared<DataFrame>();
        data->data = request->getBody();
        body->data = data;
        ok = std::dynamic_pointer_cast<Http2Stream>(stream)
                    ->m_codec->serializeTo(stream, body) > 0;
    }
    return ok;
}

http::HttpResult::ptr Http2Stream::request(http::HttpRequest::ptr req, uint64_t timeout_ms) {
    if(isConnected()) {
        Http2InitRequestForWrite(req, m_ssl);
        auto stream = newStream();
        RequestCtx::ptr ctx = std::make_shared<RequestCtx>();
        ctx->request = req;
        ctx->sn = stream->getId();
        ctx->timeout = timeout_ms;
        ctx->scheduler = sylar::Scheduler::GetThis();
        ctx->fiber = sylar::Fiber::GetThis();
        addCtx(ctx);
        ctx->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&Http2Stream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        sylar::Fiber::YieldToHold();
        auto rt = std::make_shared<http::HttpResult>(ctx->result, ctx->response, ctx->resultStr);
        if(rt->result == 0 && !ctx->response) {
            rt->result = -401;
            rt->error = "rst_stream";
        }
        return rt;
    } else {
        return std::make_shared<http::HttpResult>(AsyncSocketStream::NOT_CONNECT, nullptr, "not_connect " + getRemoteAddressString());
    }
}

void Http2Stream::handleRecvSetting(Frame::ptr frame) {
    auto s = std::dynamic_pointer_cast<SettingsFrame>(frame->data);
    SYLAR_LOG_DEBUG(g_logger) << "handleRecvSetting: " << s->toString();
    updateSettings(m_owner, s);
}

void Http2Stream::handleSendSetting(Frame::ptr frame) {
    auto s = std::dynamic_pointer_cast<SettingsFrame>(frame->data);
    updateSettings(m_peer, s);
}

int32_t Http2Stream::sendGoAway(uint32_t last_stream_id, uint32_t error, const std::string& debug) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::GOAWAY;
    GoAwayFrame::ptr data = std::make_shared<GoAwayFrame>();
    frame->data = data;
    data->last_stream_id = last_stream_id;
    data->error_code = error;
    data->data = debug;
    return sendFrame(frame);
}

int32_t Http2Stream::sendSettingsAck() {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    frame->header.flags = (uint8_t)FrameFlagSettings::ACK;
    return sendFrame(frame);
}

int32_t Http2Stream::sendSettings(const std::vector<SettingsItem>& items) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    SettingsFrame::ptr data = std::make_shared<SettingsFrame>();
    frame->data = data;
    data->items = items;
    return sendFrame(frame);
}

int32_t Http2Stream::sendRstStream(uint32_t stream_id, uint32_t error_code) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::RST_STREAM;
    frame->header.identifier = stream_id;
    RstStreamFrame::ptr data = std::make_shared<RstStreamFrame>();
    frame->data = data;
    data->error_code = error_code;
    return sendFrame(frame);
}

int32_t Http2Stream::sendPing(bool ack, uint64_t v) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::PING;
    if(ack) {
        frame->header.flags = (uint8_t)FrameFlagPing::ACK;
    }
    PingFrame::ptr data = std::make_shared<PingFrame>();
    frame->data = data;
    data->uint64 = v;
    return sendFrame(frame);
}

int32_t Http2Stream::sendWindowUpdate(uint32_t stream_id, uint32_t n) {
    //SYLAR_LOG_INFO(g_logger) << "----sendWindowUpdate id=" << stream_id << " n=" << n;
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::WINDOW_UPDATE;
    frame->header.identifier = stream_id;
    WindowUpdateFrame::ptr data = std::make_shared<WindowUpdateFrame>();
    frame->data = data;
    data->increment= n;
    //TODO update_setting
    return sendFrame(frame);
}

void Http2Stream::updateSettings(Http2Settings& sts, SettingsFrame::ptr frame) {
    DynamicTable& table = &sts == &m_owner ? m_sendTable : m_recvTable;

    for(auto& i : frame->items) {
        switch((SettingsFrame::Settings)i.identifier) {
            case SettingsFrame::Settings::HEADER_TABLE_SIZE:
                sts.header_table_size = i.value;
                table.setMaxDataSize(sts.header_table_size);
                break;
            case SettingsFrame::Settings::ENABLE_PUSH:
                if(i.value != 0 && i.value != 1) {
                    SYLAR_LOG_ERROR(g_logger) << "invalid enable_push=" << i.value;
                    sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                    //TODO close socket
                }
                sts.enable_push = i.value;
                break;
            case SettingsFrame::Settings::MAX_CONCURRENT_STREAMS:
                sts.max_concurrent_streams = i.value;
                break;
            case SettingsFrame::Settings::INITIAL_WINDOW_SIZE:
                sts.initial_window_size = i.value;
                break;
            case SettingsFrame::Settings::MAX_FRAME_SIZE:
                sts.max_frame_size = i.value;
                if(sts.max_frame_size < DEFAULT_MAX_FRAME_SIZE
                        || sts.max_frame_size > MAX_MAX_FRAME_SIZE) {
                    SYLAR_LOG_ERROR(g_logger) << "invalid max_frame_size=" << sts.max_frame_size;
                    sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                    //TODO close socket
                }
                break;
            case SettingsFrame::Settings::MAX_HEADER_LIST_SIZE:
                sts.max_header_list_size = i.value;
                break;
            default:
                //sendGoAway(m_sn, Http2Error::PROTOCOL_ERROR, "");
                //TODO close socket
                break;
        }
    }
}

http2::Stream::ptr Http2Stream::newStream(uint32_t id) {
    if(id <= m_sn) {
        return nullptr;
    }
    m_sn = id;

    http2::Stream::ptr stream = std::make_shared<http2::Stream>(
            std::dynamic_pointer_cast<Http2Stream>(shared_from_this())
            , id);
    m_streamMgr.add(stream);
    return stream;
}

http2::Stream::ptr Http2Stream::newStream() {
    http2::Stream::ptr stream = std::make_shared<http2::Stream>(
            std::dynamic_pointer_cast<Http2Stream>(shared_from_this())
            ,sylar::Atomic::addFetch(m_sn, 2));
    m_streamMgr.add(stream);
    return stream;
}

http2::Stream::ptr Http2Stream::getStream(uint32_t id) {
    return m_streamMgr.get(id);
}

void Http2Stream::delStream(uint32_t id) {
    return m_streamMgr.del(id);
}

Http2Session::Http2Session(Socket::ptr sock, Http2Server* server)
    :Http2Stream(sock, false) {
    m_server = server;
}

static bool Http2ConnectionOnConnect(AsyncSocketStream::ptr as) {
    auto stream = std::dynamic_pointer_cast<Http2Connection>(as);
    if(stream) {
        stream->reset();
        return stream->handleShakeClient();
    }
    return false;
};

Http2Connection::Http2Connection()
    :Http2Stream(nullptr, true) {
    m_autoConnect = true;
    m_connectCb = Http2ConnectionOnConnect;
}

void Http2Connection::reset() {
    m_sendTable = DynamicTable();
    m_recvTable = DynamicTable();
    m_owner = Http2Settings();
    m_peer = Http2Settings();
}

bool Http2Connection::connect(sylar::Address::ptr addr, bool ssl) {
    m_ssl = ssl;
    if(!ssl) {
        m_socket = sylar::Socket::CreateTCP(addr);
        return m_socket->connect(addr);
    } else {
        m_socket = sylar::SSLSocket::CreateTCP(addr);
        return m_socket->connect(addr);
    }
}

void Http2InitRequestForWrite(sylar::http::HttpRequest::ptr req, bool ssl) {
    req->setHeader(":scheme", (ssl ? "https" : "http"));
    if(!req->hasHeader(":path", nullptr)) {
        req->setHeader(":path", req->getUri());
    }
    req->setHeader(":method", http::HttpMethodToString(req->getMethod()));
}

void Http2InitResponseForWrite(sylar::http::HttpResponse::ptr rsp) {
    rsp->setHeader(":status", std::to_string((uint32_t)rsp->getStatus()));
}

void Http2InitRequestForRead(sylar::http::HttpRequest::ptr req) {
    req->setMethod(http::StringToHttpMethod(req->getHeader(":method")));
    if(req->hasHeader(":path", nullptr)) {
        req->setUri(req->getHeader(":path"));
        SYLAR_LOG_INFO(g_logger) << req->getPath() << " - " << req->getQuery() << " - " << req->getFragment();
    }
}

void Http2InitResponseForRead(sylar::http::HttpResponse::ptr rsp) {
    rsp->setStatus((http::HttpStatus)sylar::TypeUtil::Atoi(rsp->getHeader(":status")));
}

}
}
