#include "grpc_server.h"
#include "sylar/log.h"
#include "sylar/http/servlets/config_servlet.h"
#include "sylar/http/servlets/status_servlet.h"
#include "sylar/grpc/grpc_servlet.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

GrpcServer::GrpcServer(sylar::IOManager* worker
                        ,sylar::IOManager* io_worker
                        ,sylar::IOManager* accept_worker)
    :http2::Http2Server(worker, io_worker, accept_worker) {
    m_type = "grpc";
}

bool GrpcServer::isStreamPath(const std::string& path) {
    SYLAR_LOG_INFO(g_logger) << "size: " << m_streamTypes.size();
    for(auto& i : m_streamTypes) {
        SYLAR_LOG_INFO(g_logger) << "<<<< " << i.first << " : " << i.second;
    }
    return getStreamPathType(path) > 0;
}

bool GrpcServer::needSendResponse(const std::string& path) {
    uint32_t type = getStreamPathType(path);
    return type == 0 || type == (uint32_t)grpc::GrpcType::UNARY
        || type == (uint32_t)grpc::GrpcType::CLIENT;
}

uint32_t GrpcServer::getStreamPathType(const std::string& path) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_streamTypes.find(path);
    if(it == m_streamTypes.end()) {
        return 0;
    }
    return it->second;
}

void GrpcServer::addStreamPath(const std::string& path, uint32_t type) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_streamTypes[path] = type;
}

void GrpcServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "**** handleClient " << *client;
    sylar::grpc::GrpcSession::ptr session = std::make_shared<sylar::grpc::GrpcSession>(client, this);
    if(!session->handleShakeServer()) {
        SYLAR_LOG_WARN(g_logger) << "grpc session handleShake fail, " << session->getRemoteAddressString();
        session->close();
        return;
    }
    session->setWorker(m_worker);
    session->start();
}

void GrpcServer::addGrpcServlet(const std::string& path, GrpcServlet::ptr slt) {
    m_dispatch->addServletCreator(path, std::make_shared<CloneServletCreator>(slt));
    addStreamPath(path, (uint8_t)slt->getType());
}

}
}
