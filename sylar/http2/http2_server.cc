#include "http2_server.h"
#include "sylar/log.h"
#include "sylar/http/servlets/config_servlet.h"
#include "sylar/http/servlets/status_servlet.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Http2Server::Http2Server(const std::string& type
                        ,sylar::IOManager* worker
                        ,sylar::IOManager* io_worker
                        ,sylar::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_type = type;
    m_dispatch = std::make_shared<http::ServletDispatch>();
    m_dispatch->addServlet("/_/status", std::make_shared<http::StatusServlet>());
    m_dispatch->addServlet("/_/config", std::make_shared<http::ConfigServlet>());
}

void Http2Server::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<http::NotFoundServlet>(v));
}

void Http2Server::handleClient(Socket::ptr client) {
    SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
    sylar::http2::Http2Session::ptr session = std::make_shared<sylar::http2::Http2Session>(client, this);
    if(!session->handleShakeServer()) {
        SYLAR_LOG_WARN(g_logger) << "http2 session handleShake fail, " << session->getRemoteAddressString();
        session->close();
        return;
    }
    session->setWorker(m_worker);
    session->start();
}

}
}
