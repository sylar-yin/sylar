#include "http2_stream.h"
#include "sylar/log.h"

namespace sylar {
namespace http2 {

static const std::string CLIENT_PREFACE = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Http2Stream::Http2Stream(Socket::ptr sock, uint32_t init_sn)
    :AsyncSocketStream(sock, true)
    ,m_sn(init_sn) {
    m_codec = std::make_shared<FrameCodec>();
}

Http2Stream::~Http2Stream() {
    SYLAR_LOG_INFO(g_logger) << "Http2Stream::~Http2Stream " << this;
}

bool Http2Stream::handleShakeClient() {
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
    frame->data = std::make_shared<SettingsFrame>();

    rt = m_codec->serializeTo(shared_from_this(), frame);
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "handleShakeClient Settings fail, rt=" << rt
            << " errno=" << errno << " - " << strerror(errno);
        return false;
    }
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
    //TODO hanelSettings
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
    SYLAR_LOG_DEBUG(g_logger) << frame->toString();
    return nullptr;
}

bool Http2Stream::FrameSendCtx::doSend(AsyncSocketStream::ptr stream) {
    SYLAR_LOG_INFO(g_logger) << "doSend...";
    return std::dynamic_pointer_cast<Http2Stream>(stream)
                ->m_codec->serializeTo(stream, frame) > 0;
}

bool Http2Stream::RequestCtx::doSend(AsyncSocketStream::ptr stream) {
    SYLAR_LOG_INFO(g_logger) << "doSend...";
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
    for(auto& i : m) {
        HeaderField hf;
        hf.type = IndexType::NERVER_INDEXED_NEW_NAME;
        hf.name = i.first;
        hf.value = i.second;
        data->fields.push_back(hf);
    }
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
        RequestCtx::ptr ctx = std::make_shared<RequestCtx>();
        ctx->request = req;
        ctx->sn = sylar::Atomic::addFetch(m_sn, 2);
        ctx->timeout = timeout_ms;
        ctx->scheduler = sylar::Scheduler::GetThis();
        ctx->fiber = sylar::Fiber::GetThis();
        addCtx(ctx);
        ctx->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&Http2Stream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        sylar::Fiber::YieldToHold();
        return std::make_shared<http::HttpResult>(ctx->result, ctx->response, ctx->resultStr);
    } else {
        return std::make_shared<http::HttpResult>(AsyncSocketStream::NOT_CONNECT, nullptr, "not_connect " + getRemoteAddressString());
    }
}

}
}
