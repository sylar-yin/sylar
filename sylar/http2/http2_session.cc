#include "http2_session.h"
#include "sylar/log.h"
#include "http2_server.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Http2Session::Http2Session(Socket::ptr sock, Http2Server* server)
    :Http2SocketStream(sock, false)
    ,m_server(server) {
    SYLAR_LOG_INFO(g_logger) << "Http2Session::Http2Session sock=" << m_socket << " - " << this;
}

Http2Session::~Http2Session() {
    SYLAR_LOG_INFO(g_logger) << "Http2Session ::~Http2Session sock=" << m_socket << " - " << this;
}

void Http2Session::handleRequest(http::HttpRequest::ptr req, Http2Stream::ptr stream) {
    if(stream->getHandleCount() > 0) {
        return;
    }
    stream->addHandleCount();
    http::HttpResponse::ptr rsp = std::make_shared<http::HttpResponse>(req->getVersion(), false);
    req->setStreamId(stream->getId());
    SYLAR_LOG_DEBUG(g_logger) << *req;
    rsp->setHeader("server", m_server->getName());
    m_server->getServletDispatch()->handle(req, rsp, shared_from_this());
    stream->sendResponse(rsp, true, true);
    delStream(stream->getId());
}

AsyncSocketStream::Ctx::ptr Http2Session::onStreamClose(Http2Stream::ptr stream) {
    auto req = stream->getRequest();
    if(!req) {
        SYLAR_LOG_DEBUG(g_logger) << "Http2Session recv http request fail, errno="
            << errno << " errstr=" << strerror(errno) << " - " << getRemoteAddressString();
        sendGoAway(m_sn, (uint32_t)Http2Error::PROTOCOL_ERROR, "");
        delStream(stream->getId());
        return nullptr;
    }
    if(stream->getHandleCount() == 0) {
        m_worker->schedule(std::bind(&Http2Session::handleRequest, getSelf(), req, stream));
    }
    return nullptr;
}

AsyncSocketStream::Ctx::ptr Http2Session::onHeaderEnd(Http2Stream::ptr stream) {
    return nullptr;
}

Http2Session::ptr Http2Session::getSelf() {
    return std::dynamic_pointer_cast<Http2Session>(shared_from_this());
}

}
}
