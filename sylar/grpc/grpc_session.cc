#include "grpc_session.h"
#include "sylar/log.h"
#include "grpc_server.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

GrpcSession::GrpcSession(Socket::ptr sock, GrpcServer* server)
    :http2::Http2SocketStream(sock, false)
    ,m_server(server) {
    SYLAR_LOG_INFO(g_logger) << "GrpcSession::GrpcSession sock=" << m_socket << " - " << this;
}

GrpcSession::~GrpcSession() {
    SYLAR_LOG_INFO(g_logger) << "GrpcSession ::~GrpcSession sock=" << m_socket << " - " << this;
}

void GrpcSession::handleRequest(http::HttpRequest::ptr req, http2::Http2Stream::ptr stream) {
    if(stream->getHandleCount() > 0) {
        return;
    }
    stream->addHandleCount();
    http::HttpResponse::ptr rsp = std::make_shared<http::HttpResponse>(req->getVersion(), false);
    req->setStreamId(stream->getId());
    SYLAR_LOG_DEBUG(g_logger) << *req;
    rsp->setHeader("server", m_server->getName());
    int rt = m_server->getServletDispatch()->handle(req, rsp, shared_from_this());
    if(rt != 0 || m_server->needSendResponse(req->getPath())) {
        SYLAR_LOG_INFO(g_logger) << "send response ======";
        stream->sendResponse(rsp, true, true);
    }
    delStream(stream->getId());
}

AsyncSocketStream::Ctx::ptr GrpcSession::onStreamClose(http2::Http2Stream::ptr stream) {
    auto req = stream->getRequest();
    if(!req) {
        SYLAR_LOG_DEBUG(g_logger) << "GrpcSession recv http request fail, errno="
            << errno << " errstr=" << strerror(errno) << " - " << getRemoteAddressString();
        sendGoAway(m_sn, (uint32_t)http2::Http2Error::PROTOCOL_ERROR, "");
        delStream(stream->getId());
        return nullptr;
    }
    if(stream->getHandleCount() == 0) {
        m_worker->schedule(std::bind(&GrpcSession::handleRequest, getSelf(), req, stream));
    }
    return nullptr;
}

AsyncSocketStream::Ctx::ptr GrpcSession::onHeaderEnd(http2::Http2Stream::ptr stream) {
    auto path = stream->getHeader(":path");
    SYLAR_LOG_INFO(g_logger) << " ******* header *********" << path
        << " - " << (int)(!path.empty())
        << " - " << (int)m_server->isStreamPath(path)
        << " - " << (int)(stream->getHandleCount() == 0);
    if(!path.empty() && m_server->isStreamPath(path) && stream->getHandleCount() == 0) {
        stream->setIsStream(true);
        stream->initRequest();
        auto req = stream->getRequest();
        if(stream->getHandleCount() == 0) {
            m_worker->schedule(std::bind(&GrpcSession::handleRequest, getSelf(), req, stream));
        }
    }
    return nullptr;
}

GrpcSession::ptr GrpcSession::getSelf() {
    return std::dynamic_pointer_cast<GrpcSession>(shared_from_this());
}
}
}
