#include "http2_connection.h"
#include "sylar/log.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static bool Http2ConnectionOnConnect(AsyncSocketStream::ptr as) {
    //sleep(10);
    auto stream = std::dynamic_pointer_cast<Http2Connection>(as);
    if(stream) {
        stream->reset();
        return stream->handleShakeClient();
    }
    return false;
};

Http2Connection::Http2Connection()
    :Http2SocketStream(nullptr, true) {
    m_autoConnect = true;
    m_connectCb = Http2ConnectionOnConnect;
    SYLAR_LOG_INFO(g_logger) << "Http2Connection::Http2Connection sock=" << m_socket << " - " << this;
}

Http2Connection::~Http2Connection() {
    SYLAR_LOG_INFO(g_logger) << "Http2Connection::~Http2Connection sock=" << m_socket << " - " << this;
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

void Http2Connection::reset() {
    m_sendTable = DynamicTable();
    m_recvTable = DynamicTable();
    m_owner = Http2Settings();
    m_peer = Http2Settings();
    m_sendWindow = DEFAULT_INITIAL_WINDOW_SIZE;
    m_recvWindow = DEFAULT_INITIAL_WINDOW_SIZE;
    m_sn = (m_isClient ? -1 : 0);
    m_streamMgr.clear();
}

AsyncSocketStream::Ctx::ptr Http2Connection::onStreamClose(Http2Stream::ptr stream) {
    delStream(stream->getId());
    RequestCtx::ptr ctx = getAndDelCtxAs<RequestCtx>(stream->getId());
    if(!ctx) {
        SYLAR_LOG_WARN(g_logger) << "Http2Connection request timeout response - " << getRemoteAddressString();
        return nullptr;
    }
    ctx->response = stream->getResponse();
    return ctx;
}

AsyncSocketStream::Ctx::ptr Http2Connection::onHeaderEnd(Http2Stream::ptr stream) {
    return nullptr;
}

}
}
