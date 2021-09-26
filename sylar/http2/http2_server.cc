#include "http2_server.h"
#include "sylar/log.h"
#include "sylar/http/servlets/config_servlet.h"
#include "sylar/http/servlets/status_servlet.h"
#include "sylar/grpc/grpc_servlet.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Http2Server::Http2Server(sylar::IOManager* worker
                        ,sylar::IOManager* io_worker
                        ,sylar::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_type = "http2";
    m_dispatch = std::make_shared<http::ServletDispatch>();
    m_dispatch->addServlet("/_/status", std::make_shared<http::StatusServlet>());
    m_dispatch->addServlet("/_/config", std::make_shared<http::ConfigServlet>());
}

void Http2Server::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<http::NotFoundServlet>(v));
}

bool Http2Server::isStreamPath(const std::string& path) {
    SYLAR_LOG_INFO(g_logger) << "size: " << m_streamTypes.size();
    for(auto& i : m_streamTypes) {
        SYLAR_LOG_INFO(g_logger) << "<<<< " << i.first << " : " << i.second;
    }
    return getStreamPathType(path) > 0;
}

bool Http2Server::needSendResponse(const std::string& path) {
    uint32_t type = getStreamPathType(path);
    return type == 0 || type == (uint32_t)grpc::GrpcType::UNARY
        || type == (uint32_t)grpc::GrpcType::CLIENT;
}

uint32_t Http2Server::getStreamPathType(const std::string& path) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_streamTypes.find(path);
    if(it == m_streamTypes.end()) {
        return 0;
    }
    return it->second;
}

void Http2Server::addStreamPath(const std::string& path, uint32_t type) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_streamTypes[path] = type;
}

void Http2Server::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "**** handleClient " << *client;
    sylar::http2::Http2Session::ptr session = std::make_shared<sylar::http2::Http2Session>(client, this);
    if(!session->handleShakeServer()) {
        SYLAR_LOG_WARN(g_logger) << "http2 session handleShake fail, " << session->getRemoteAddressString();
        session->close();
        return;
    }
    session->setWorker(m_worker);
    session->start();
}

void Http2Server::addGrpcServlet(const std::string& path, sylar::http::Servlet::ptr slt) {
    m_dispatch->addServlet(path, slt);
}

void Http2Server::addGrpcStreamClientServlet(const std::string& path, sylar::http::Servlet::ptr slt) {
    m_dispatch->addServlet(path, slt);
    addStreamPath(path, (uint8_t)sylar::grpc::GrpcType::CLIENT);
}

void Http2Server::addGrpcStreamServerServlet(const std::string& path, sylar::http::Servlet::ptr slt) {
    m_dispatch->addServlet(path, slt);
    addStreamPath(path, (uint8_t)sylar::grpc::GrpcType::SERVER);
}

void Http2Server::addGrpcStreamBothServlet(const std::string& path, sylar::http::Servlet::ptr slt) {
    m_dispatch->addServlet(path, slt);
    addStreamPath(path, (uint8_t)sylar::grpc::GrpcType::BOTH);
}

}
}
