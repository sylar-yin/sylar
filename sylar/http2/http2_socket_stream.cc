#include "http2_socket_stream.h"
#include "sylar/log.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static const std::string CLIENT_PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

Http2SocketStream::Http2SocketStream(Socket::ptr sock, bool client)
    :AsyncSocketStream(sock, true)
    ,m_sn(client ? -1 : 0)
    ,m_isClient(client)
    ,m_ssl(false) {
    m_codec = std::make_shared<FrameCodec>();
    SYLAR_LOG_INFO(g_logger) << "Http2SocketStream::Http2SocketStream sock=" << sock << " - " << this;
}

Http2SocketStream::~Http2SocketStream() {
    SYLAR_LOG_INFO(g_logger) << "Http2SocketStream::~Http2SocketStream " << this;
}

void Http2SocketStream::onClose() {
    SYLAR_LOG_INFO(g_logger) << "******** onClose " << getLocalAddressString() << " - " << getRemoteAddressString();
    m_streamMgr.clear();
}

bool Http2SocketStream::handleShakeClient() {
    SYLAR_LOG_INFO(g_logger) << "handleShakeClient " << getRemoteAddressString();
    if(!isConnected()) {
        return false;
    }

    int rt = writeFixSize(CLIENT_PREFACE.c_str(), CLIENT_PREFACE.size());
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeClient CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno) << " - " << getRemoteAddressString();
        return false;
    }

    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    auto sf = std::make_shared<SettingsFrame>();
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::ENABLE_PUSH, 0);
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::INITIAL_WINDOW_SIZE, 4194304);
    sf->items.emplace_back((uint8_t)SettingsFrame::Settings::MAX_HEADER_LIST_SIZE, 10485760);
    frame->data = sf;

    handleSendSetting(frame);
    rt = sendFrame(frame, false);
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeClient Settings fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno) << " - " << getRemoteAddressString();
        return false;
    }
    sendWindowUpdate(0, MAX_INITIAL_WINDOW_SIZE - m_recvWindow);
    return true;
}

bool Http2SocketStream::handleShakeServer() {
    ByteArray::ptr ba = std::make_shared<ByteArray>();
    int rt = readFixSize(ba, CLIENT_PREFACE.size());
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno) << " - " << getRemoteAddressString();
        return false;
    }
    ba->setPosition(0);
    if(ba->toString() != CLIENT_PREFACE) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv CLIENT_PREFACE fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno)
            << " hex: " << ba->toHexString() << " - " << getRemoteAddressString();
        return false;
    }
    auto frame = m_codec->parseFrom(shared_from_this());
    if(!frame) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv SettingsFrame fail,"
            << " errno=" << errno << " - " << strerror(errno) << " - " << getRemoteAddressString();
        return false;
    }
    if(frame->header.type != (uint8_t)FrameType::SETTINGS) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeServer recv Frame not SettingsFrame, type="
            << FrameTypeToString((FrameType)frame->header.type) << " - " << getRemoteAddressString();
        return false;
    }
    handleRecvSetting(frame);
    sendSettingsAck();
    sendSettings({});
    return true;
}

int32_t Http2SocketStream::sendFrame(Frame::ptr frame, bool async) {
    if(isConnected()) {
        if(async) {
            FrameSendCtx::ptr ctx = std::make_shared<FrameSendCtx>();
            ctx->frame = frame;
            enqueue(ctx);
            return 1;
        } else {
            return m_codec->serializeTo(shared_from_this(), frame);
        }
    } else {
        return -1;
    }
}

void Http2SocketStream::handleWindowUpdate(Frame::ptr frame) {
    auto wuf = std::dynamic_pointer_cast<WindowUpdateFrame>(frame->data);
    if(wuf) {
        if(frame->header.identifier) {
            auto stream = getStream(frame->header.identifier);
            if(!stream) {
                SYLAR_LOG_ERROR(g_logger) << "WINDOW_UPDATE stream_id=" << frame->header.identifier
                    << " not exists, " << getRemoteAddressString();
                sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                return;
            }
            if(((int64_t)stream->m_sendWindow + wuf->increment) > MAX_INITIAL_WINDOW_SIZE) {
                SYLAR_LOG_ERROR(g_logger) << "WINDOW_UPDATE stream_id=" << stream->getId()
                    << " increment=" << wuf->increment << " send_window=" << stream->m_sendWindow
                    << " biger than " << MAX_INITIAL_WINDOW_SIZE << " " << getRemoteAddressString();
                sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                return;
            }
            stream->updateSendWindowByDiff(wuf->increment);
        } else {
            if(((int64_t)m_sendWindow + wuf->increment) > MAX_INITIAL_WINDOW_SIZE) {
                SYLAR_LOG_ERROR(g_logger) << "WINDOW_UPDATE stream_id=0"
                    << " increment=" << wuf->increment << " send_window=" << m_sendWindow
                    << " biger than " << MAX_INITIAL_WINDOW_SIZE << " " << getRemoteAddressString();
                sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                return;
            }
            m_sendWindow += wuf->increment;
        }
    } else {
        SYLAR_LOG_ERROR(g_logger) << "WINDOW_UPDATE stream_id=" << frame->header.identifier
            << " invalid body " << getRemoteAddressString();
        innerClose();
    }
}

void Http2SocketStream::handleRecvData(Frame::ptr frame, Http2Stream::ptr stream) {
    if(frame->header.length) {
        m_recvWindow -= frame->header.length;
        if(m_recvWindow < (int32_t)MAX_INITIAL_WINDOW_SIZE / 4) {
            SYLAR_LOG_INFO(g_logger) << "recv_window=" << m_recvWindow
                << " length=" << frame->header.length
                << " " << (MAX_INITIAL_WINDOW_SIZE / 4);
            sendWindowUpdate(0, MAX_INITIAL_WINDOW_SIZE - m_recvWindow);
        }

        stream->m_recvWindow -= frame->header.length;
        if(stream->m_recvWindow < (int32_t)MAX_INITIAL_WINDOW_SIZE / 4) {
            SYLAR_LOG_INFO(g_logger) << "recv_window=" << stream->m_recvWindow
                << " length=" << frame->header.length
                << " " << (MAX_INITIAL_WINDOW_SIZE / 4)
                << " diff=" << (MAX_INITIAL_WINDOW_SIZE - stream->m_recvWindow);
            sendWindowUpdate(stream->getId(), MAX_INITIAL_WINDOW_SIZE - stream->m_recvWindow);
        }
    }
}

struct ScopeTest {
    ScopeTest() {
        SYLAR_LOG_INFO(g_logger) << "=========== DoRecv ===========";
    }
    ~ScopeTest() {
        SYLAR_LOG_INFO(g_logger) << "=========== DoRecv Out ===========";
    }
};

AsyncSocketStream::Ctx::ptr Http2SocketStream::doRecv() {
    //ScopeTest xxx;
    //SYLAR_LOG_INFO(g_logger) << "=========== DoRecv ===========";
    auto frame = m_codec->parseFrom(shared_from_this());
    if(!frame) {
        innerClose();
        return nullptr;
    }
    SYLAR_LOG_DEBUG(g_logger) << getRemoteAddressString() << " recv: " << frame->toString();
    //TODO handle RST_STREAM

    if(frame->header.type == (uint8_t)FrameType::WINDOW_UPDATE) {
        handleWindowUpdate(frame);
    } else if(frame->header.identifier) {
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
                    if(frame->header.type != (uint8_t)FrameType::RST_STREAM) {
                        //sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                        sendRstStream(m_sn, (uint32_t)Http2Error::STREAM_CLOSED_ERROR);
                    }
                    return nullptr;
                }
            }
        }

        if(frame->header.type == (uint8_t)FrameType::DATA) {
            handleRecvData(frame, stream);
        }
        stream->handleFrame(frame, m_isClient);
        if(stream->getState() == Http2Stream::State::CLOSED) {
            return onStreamClose(stream);
        } else if(frame->header.type == (uint8_t)FrameType::HEADERS
                    && frame->header.flags & (uint8_t)FrameFlagHeaders::END_HEADERS) {
            return onHeaderEnd(stream);
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

//void Http2SocketStream::handleRequest(http::HttpRequest::ptr req, Http2Stream::ptr stream) {
//    if(stream->getHandleCount() > 0) {
//        return;
//    }
//    stream->addHandleCount();
//    http::HttpResponse::ptr rsp = std::make_shared<http::HttpResponse>(req->getVersion(), false);
//    req->setStreamId(stream->getId());
//    SYLAR_LOG_DEBUG(g_logger) << *req;
//    rsp->setHeader("server", m_server->getName());
//    int rt = m_server->getServletDispatch()->handle(req, rsp, shared_from_this());
//    if(rt != 0 || m_server->needSendResponse(req->getPath())) {
//        SYLAR_LOG_INFO(g_logger) << "send response ======";
//        stream->sendResponse(rsp);
//    }
//    delStream(stream->getId());
//}

bool Http2SocketStream::FrameSendCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<Http2SocketStream>(stream)
                ->sendFrame(frame, false) > 0;
}

int32_t Http2SocketStream::sendData(Http2Stream::ptr stream, const std::string& data, bool async, bool end_stream) {
    int pos = 0;
    int length = data.size();

    //m_peer.max_frame_size = 1024;
    auto max_frame_size = m_peer.max_frame_size - 9;

    do {
        int len = length;
        if(len > (int)max_frame_size) {
            len = max_frame_size;
        }
        Frame::ptr body = std::make_shared<Frame>();
        body->header.type = (uint8_t)FrameType::DATA;
        if(end_stream) {
            body->header.flags = (length == len ? (uint8_t)FrameFlagData::END_STREAM : 0);
        } else {
            body->header.flags = 0;
        }
        body->header.identifier = stream->getId();
        auto df = std::make_shared<DataFrame>();
        df->data = data.substr(pos, len);
        body->data = df;

        int rt = sendFrame(body, async);
        if(rt <= 0) {
            SYLAR_LOG_DEBUG(g_logger) << "sendData error rt=" << rt << " errno=" << errno << " - " << getRemoteAddressString();
            return rt;
        }
        length -= len;
        pos += len;

        stream->updateSendWindowByDiff(-len);
        sylar::Atomic::addFetch(m_sendWindow, -len);
        //send_window -= len;
    } while(length > 0);
    return 1;
}

bool Http2SocketStream::RequestCtx::doSend(AsyncSocketStream::ptr stream) {
    auto h2stream = std::dynamic_pointer_cast<Http2SocketStream>(stream);
    auto stm = h2stream->getStream(sn);
    if(!stm) {
        SYLAR_LOG_ERROR(g_logger) << "RequestCtx doSend Fail, sn=" << sn
            << " not exists - " << stream->getRemoteAddressString();
        return false;
    }
    return stm->sendRequest(request, true, false);
}

bool Http2SocketStream::StreamCtx::doSend(AsyncSocketStream::ptr stream) {
    auto h2stream = std::dynamic_pointer_cast<Http2SocketStream>(stream);
    auto stm = h2stream->getStream(sn);
    if(!stm) {
        SYLAR_LOG_ERROR(g_logger) << "StreamCtx doSend Fail, sn=" << sn
            << " not exists - " << stream->getRemoteAddressString();
        return false;
    }
    return stm->sendRequest(request, false, false);
}

void Http2SocketStream::onTimeOut(AsyncSocketStream::Ctx::ptr ctx) {
    AsyncSocketStream::onTimeOut(ctx);
    delStream(ctx->sn);
}

//StreamClient::ptr Http2SocketStream::openStreamClient(sylar::http::HttpRequest::ptr request) {
//    if(isConnected()) {
//        //Http2InitRequestForWrite(req, m_ssl);
//        auto stream = newStream();
//        StreamCtx::ptr ctx = std::make_shared<StreamCtx>();
//        ctx->request = request;
//        ctx->sn = stream->getId();
//        enqueue(ctx);
//        return StreamClient::Create(stream);
//    }
//    return nullptr;
//}

Http2Stream::ptr Http2SocketStream::openStream(sylar::http::HttpRequest::ptr request) {
    if(isConnected()) {
        //Http2InitRequestForWrite(req, m_ssl);
        auto stream = newStream();
        stream->m_isStream = true;
        StreamCtx::ptr ctx = std::make_shared<StreamCtx>();
        ctx->request = request;
        ctx->sn = stream->getId();
        enqueue(ctx);
        return stream;
    }
    return nullptr;
}

http::HttpResult::ptr Http2SocketStream::request(http::HttpRequest::ptr req, uint64_t timeout_ms) {
    if(isConnected()) {
        //Http2InitRequestForWrite(req, m_ssl);
        auto stream = newStream();
        RequestCtx::ptr ctx = std::make_shared<RequestCtx>();
        ctx->request = req;
        ctx->sn = stream->getId();
        ctx->timeout = timeout_ms;
        ctx->scheduler = sylar::Scheduler::GetThis();
        ctx->fiber = sylar::Fiber::GetThis();
        addCtx(ctx);
        ctx->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&Http2SocketStream::onTimeOut, std::dynamic_pointer_cast<Http2SocketStream>(shared_from_this()), ctx));
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

void Http2SocketStream::handleRecvSetting(Frame::ptr frame) {
    auto s = std::dynamic_pointer_cast<SettingsFrame>(frame->data);
    SYLAR_LOG_DEBUG(g_logger) << "handleRecvSetting: " << s->toString();
    updateSettings(m_owner, s);
}

void Http2SocketStream::handleSendSetting(Frame::ptr frame) {
    auto s = std::dynamic_pointer_cast<SettingsFrame>(frame->data);
    updateSettings(m_peer, s);
}

int32_t Http2SocketStream::sendGoAway(uint32_t last_stream_id, uint32_t error, const std::string& debug) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::GOAWAY;
    GoAwayFrame::ptr data = std::make_shared<GoAwayFrame>();
    frame->data = data;
    data->last_stream_id = last_stream_id;
    data->error_code = error;
    data->data = debug;
    return sendFrame(frame, true);
}

int32_t Http2SocketStream::sendSettingsAck() {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    frame->header.flags = (uint8_t)FrameFlagSettings::ACK;
    return sendFrame(frame, true);
}

int32_t Http2SocketStream::sendSettings(const std::vector<SettingsItem>& items) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::SETTINGS;
    SettingsFrame::ptr data = std::make_shared<SettingsFrame>();
    frame->data = data;
    data->items = items;
    int rt = sendFrame(frame, true);
    if(rt > 0) {
        handleSendSetting(frame);
    }
    return rt;
}

int32_t Http2SocketStream::sendRstStream(uint32_t stream_id, uint32_t error_code) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::RST_STREAM;
    frame->header.identifier = stream_id;
    RstStreamFrame::ptr data = std::make_shared<RstStreamFrame>();
    frame->data = data;
    data->error_code = error_code;
    return sendFrame(frame, true);
}

int32_t Http2SocketStream::sendPing(bool ack, uint64_t v) {
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::PING;
    if(ack) {
        frame->header.flags = (uint8_t)FrameFlagPing::ACK;
    }
    PingFrame::ptr data = std::make_shared<PingFrame>();
    frame->data = data;
    data->uint64 = v;
    return sendFrame(frame, true);
}

int32_t Http2SocketStream::sendWindowUpdate(uint32_t stream_id, uint32_t n) {
    //SYLAR_LOG_INFO(g_logger) << "----sendWindowUpdate id=" << stream_id << " n=" << n;
    Frame::ptr frame = std::make_shared<Frame>();
    frame->header.type = (uint8_t)FrameType::WINDOW_UPDATE;
    frame->header.identifier = stream_id;
    WindowUpdateFrame::ptr data = std::make_shared<WindowUpdateFrame>();
    frame->data = data;
    data->increment= n;
    if(stream_id == 0) {
        m_recvWindow += n;
    } else {
        auto stm = getStream(stream_id);
        if(stm) {
            stm->updateRecvWindowByDiff(n);
        } else {
            SYLAR_LOG_ERROR(g_logger) << "sendWindowUpdate stream=" << stream_id << " not exists";
        }
    }
    return sendFrame(frame, true);
}

void Http2SocketStream::updateSettings(Http2Settings& sts, SettingsFrame::ptr frame) {
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
                if(i.value > MAX_INITIAL_WINDOW_SIZE) {
                    SYLAR_LOG_ERROR(g_logger) << "INITIAL_WINDOW_SIZE invalid value=" << i.value;
                    sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
                } else {
                    int32_t diff = i.value - sts.initial_window_size;
                    sts.initial_window_size = i.value;
                    if(&sts == &m_peer) {
                        updateRecvWindowByDiff(diff);
                    } else {
                        updateSendWindowByDiff(diff);
                    }
                }
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

Http2Stream::ptr Http2SocketStream::newStream(uint32_t id) {
    if(id <= m_sn) {
        return nullptr;
    }
    m_sn = id;

    Http2Stream::ptr stream = std::make_shared<Http2Stream>(
            std::dynamic_pointer_cast<Http2SocketStream>(shared_from_this())
            , id);
    m_streamMgr.add(stream);
    return stream;
}

Http2Stream::ptr Http2SocketStream::newStream() {
    Http2Stream::ptr stream = std::make_shared<Http2Stream>(
            std::dynamic_pointer_cast<Http2SocketStream>(shared_from_this())
            ,sylar::Atomic::addFetch(m_sn, 2));
    m_streamMgr.add(stream);
    return stream;
}

Http2Stream::ptr Http2SocketStream::getStream(uint32_t id) {
    return m_streamMgr.get(id);
}

void Http2SocketStream::delStream(uint32_t id) {
    auto stream = m_streamMgr.get(id);
    if(stream) {
        stream->close();
    }
    return m_streamMgr.del(id);
}

void Http2SocketStream::updateSendWindowByDiff(int32_t diff) {
    m_streamMgr.foreach([diff, this](Http2Stream::ptr stream){
        if(stream->updateSendWindowByDiff(diff)) {
            sendRstStream(stream->getId(), (uint32_t)Http2Error::FLOW_CONTROL_ERROR);
        }
    });
}

void Http2SocketStream::updateRecvWindowByDiff(int32_t diff) {
    m_streamMgr.foreach([this, diff](Http2Stream::ptr stream){
        if(stream->updateRecvWindowByDiff(diff)) {
            sendRstStream(stream->getId(), (uint32_t)Http2Error::FLOW_CONTROL_ERROR);
        }
    });
}

}
}
