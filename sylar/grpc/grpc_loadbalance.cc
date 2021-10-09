#include "grpc_loadbalance.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/worker.h"
#include "sylar/streams/zlib_stream.h"
#include "grpc_util.h"
#include "grpc_connection.h"
#include <sstream>

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> > >::ptr g_grpc_services =
    sylar::Config::Lookup("grpc_services", std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> >(), "grpc_services");

GrpcSDLoadBalance::GrpcSDLoadBalance(IServiceDiscovery::ptr sd)
    :SDLoadBalance(sd) {
    m_type = "grpc";
}

static SocketStream::ptr create_grpc_stream(const std::string& domain, const std::string& service, ServiceItemInfo::ptr info) {
    //SYLAR_LOG_INFO(g_logger) << "create_grpck_stream: " << info->toString();
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress(info->getIp());
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "invalid service info: " << info->toString();
        return nullptr;
    }
    addr->setPort(info->getPort());
    GrpcConnection::ptr conn = std::make_shared<GrpcConnection>();

    sylar::WorkerMgr::GetInstance()->schedule("service_io", [conn, addr, domain, service](){
        conn->connect(addr);
        //SYLAR_LOG_INFO(g_logger) << *addr << " - " << domain << " - " << service;
        conn->start();
    });
    return conn;
}

void GrpcSDLoadBalance::start() {
    m_cb = create_grpc_stream;
    initConf(g_grpc_services->getValue());
    SDLoadBalance::start();
}

void GrpcSDLoadBalance::start(const std::unordered_map<std::string
                              ,std::unordered_map<std::string,std::string> >& confs) {
    m_cb = create_grpc_stream;
    initConf(confs);
    SDLoadBalance::start();
}

void GrpcSDLoadBalance::stop() {
    SDLoadBalance::stop();
}

GrpcResponse::ptr GrpcSDLoadBalance::request(const std::string& domain, const std::string& service,
                                           GrpcRequest::ptr req, uint32_t timeout_ms, uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return std::make_shared<GrpcResponse>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        return std::make_shared<GrpcResponse>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
    }
    uint64_t ts = sylar::GetCurrentMS();
    auto& stats = conn->get(ts / 1000);
    stats.incDoing(1);
    stats.incTotal(1);
    auto r = conn->getStreamAs<GrpcConnection>()->request(req, timeout_ms);
    uint64_t ts2 = sylar::GetCurrentMS();
    r->setUsed(ts2 - ts);
    if(r->getResult() == 0) {
        stats.incOks(1);
        stats.incUsedTime(ts2 -ts);
    } else if(r->getResult() == AsyncSocketStream::TIMEOUT) {
        stats.incTimeouts(1);
    } else if(r->getResult() < 0) {
        stats.incErrs(1);
    }
    stats.decDoing(1);
    return r;
}

GrpcResponse::ptr GrpcSDLoadBalance::request(const std::string& domain, const std::string& service,
                                           const std::string& method, const google::protobuf::Message& message,
                                           uint32_t timeout_ms,
                                           const std::map<std::string, std::string>& headers,
                                           uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return std::make_shared<GrpcResponse>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        return std::make_shared<GrpcResponse>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
    }
    uint64_t ts = sylar::GetCurrentMS();
    auto& stats = conn->get(ts / 1000);
    stats.incDoing(1);
    stats.incTotal(1);
    auto r = conn->getStreamAs<GrpcConnection>()->request(method, message, timeout_ms, headers);
    uint64_t ts2 = sylar::GetCurrentMS();
    r->setUsed(ts2 - ts);
    if(r->getResult() == 0) {
        stats.incOks(1);
        stats.incUsedTime(ts2 -ts);
    } else if(r->getResult() == AsyncSocketStream::TIMEOUT) {
        stats.incTimeouts(1);
    } else if(r->getResult() < 0) {
        stats.incErrs(1);
    }
    stats.decDoing(1);
    return r;
}


GrpcStream::ptr GrpcSDLoadBalance::openStream(const std::string& domain, const std::string& service,
                                 sylar::http::HttpRequest::ptr request, uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return nullptr;
        //return std::make_shared<GrpcResponse>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        //return std::make_shared<GrpcResponse>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
        return nullptr;
    }

    auto cli = conn->getStreamAs<GrpcConnection>()->openStream(request);
    if(cli) {
        return std::make_shared<GrpcStream>(cli);
    }
    return nullptr;
}

/*
GrpcStreamBase::GrpcStreamBase(GrpcStreamClient::ptr client)
        :m_client(client) {
}

int32_t GrpcStreamBase::close(int err) {
    if(!m_client) {
        return -1;
    }
    auto stm = m_client->getStream();
    if(!stm) {
        return -2;
    }
    auto hs = stm->getSockStream();
    if(!hs) {
        return -3;
    }
    return hs->sendRstStream(stm->getId(), err);
}
*/

}
}
