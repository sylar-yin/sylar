#include "grpc_stream.h"
#include "sylar/log.h"
#include "sylar/worker.h"
#include <sstream>

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> > >::ptr g_grpc_services =
    sylar::Config::Lookup("grpc_services", std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> >(), "grpc_services");

sylar::ByteArray::ptr GrpcMessage::packData() const {
    sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();
    ba->writeFuint8(0);
    ba->writeFuint32(data.size());
    ba->write(data.c_str(), data.size());
    return ba;
}

std::string GrpcRequest::toString() const {
    std::stringstream ss;
    ss << "[GrpcRequest request=" << m_request
       << " data=" << m_data << "]";
    return ss.str();
}

std::string GrpcResult::toString() const {
    std::stringstream ss;
    ss << "[GrpcResult result=" << m_result
       << " error=" << m_error
       << " used=" << m_used
       << " response=" << m_response << "]";
    return ss.str();
}

GrpcResult::ptr GrpcConnection::request(GrpcRequest::ptr req, uint64_t timeout_ms) {
    auto http_req = req->getRequest();
    http_req->setHeader("content-type", "application/grpc+proto");
    auto grpc_data = req->getData();
    std::string data;
    data.resize(grpc_data->data.size() + 5);
    sylar::ByteArray::ptr ba(new sylar::ByteArray(&data[0], data.size()));
    ba->writeFuint8(0);
    ba->writeFuint32(grpc_data->data.size());
    ba->write(grpc_data->data.c_str(), grpc_data->data.size());
    http_req->setBody(data);

    GrpcResult::ptr rsp = std::make_shared<GrpcResult>();
    auto result = Http2Connection::request(http_req, timeout_ms);
    SYLAR_LOG_DEBUG(g_logger) << "send data.size=" << data.size();
    rsp->setResult(result->result);
    rsp->setError(result->error);
    rsp->setResponse(result->response);
    if(!result->result) {
        rsp->setResult(sylar::TypeUtil::Atoi(result->response->getHeader("grpc-status")));
        rsp->setError(result->response->getHeader("grpc-message"));

        auto& body = result->response->getBody();
        if(!body.empty()) {
            sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
            GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
            rsp->setData(msg);

            try {
                msg->compressed = ba->readFuint8();
                msg->length = ba->readFuint32();
                msg->data = ba->toString();
            } catch (...) {
                SYLAR_LOG_ERROR(g_logger) << "invalid grpc body";
                result->result = -501;
                result->error = "invalid grpc body";
            }
        }
    }
    return rsp;
}

GrpcResult::ptr GrpcConnection::request(const std::string& method
        ,const google::protobuf::Message& message, uint64_t timeout_ms
        ,const std::map<std::string, std::string>& headers) {
    GrpcRequest::ptr grpc_req = std::make_shared<GrpcRequest>();
    http::HttpRequest::ptr http_req = std::make_shared<http::HttpRequest>(0x20, false);
    grpc_req->setRequest(http_req);
    http_req->setMethod(http::HttpMethod::POST);
    http_req->setPath(method);
    for(auto & i : headers) {
        http_req->setHeader(i.first, i.second);
    }
    grpc_req->setAsPB(message);

    //GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
    //msg->compressed = 0;
    //if(!message.SerializeToString(&msg->data)) {
    //    SYLAR_LOG_ERROR(g_logger) << "SerializeToString fail, " << sylar::PBToJsonString(message);
    //    GrpcResult::ptr rsp = std::make_shared<GrpcResult>();
    //    rsp->setResult(-100);
    //    rsp->setError("pb SerializeToString fail");
    //    return rsp;
    //}
    //msg->compressed = msg->data.size();
    //grpc_req->setData(msg);
    return request(grpc_req, timeout_ms);
}

GrpcConnection::GrpcConnection() {
    //m_autoConnect = false;
}

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

GrpcResult::ptr GrpcSDLoadBalance::request(const std::string& domain, const std::string& service,
                                           GrpcRequest::ptr req, uint32_t timeout_ms, uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return std::make_shared<GrpcResult>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        return std::make_shared<GrpcResult>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
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

GrpcResult::ptr GrpcSDLoadBalance::request(const std::string& domain, const std::string& service,
                                           const std::string& method, const google::protobuf::Message& message,
                                           uint32_t timeout_ms,
                                           const std::map<std::string, std::string>& headers,
                                           uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return std::make_shared<GrpcResult>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        return std::make_shared<GrpcResult>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
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


GrpcStreamClient::GrpcStreamClient(http2::StreamClient::ptr client)
    : m_client(client) {
}

http2::Stream::ptr GrpcStreamClient::getStream() {
    return m_client->getStream();
}

int32_t GrpcStreamClient::sendData(const std::string& data, bool end_stream) {
    return m_client->sendData(data, end_stream);
}

http2::DataFrame::ptr GrpcStreamClient::recvData() {
    return m_client->recvData();
}

int32_t GrpcStreamClient::sendMessage(const google::protobuf::Message& msg, bool end_stream) {
    GrpcMessage m;
    msg.SerializeToString(&m.data);
    auto ba = m.packData();
    ba->setPosition(0);
    return m_client->sendData(ba->toString(), end_stream);
}

std::shared_ptr<std::string> GrpcStreamClient::recvMessageData() {
    try {
        auto df = recvData();
        if(!df) {
            return nullptr;
        }

        auto rt = std::make_shared<std::string>();
        auto& body = df->data;
        if(body.empty()) {
            return nullptr;
        }
        sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
        bool compress = ba->readFuint8();
        (void)compress;
        uint32_t length = ba->readFuint32();
        *rt += ba->toString();
        while(rt->size() < length) {
            df = recvData();
            if(!df) {
                return nullptr;
            }
            *rt += df->data;
        }
        return rt;
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "recvMessageData error";
    }
    return nullptr;
}

GrpcStreamClient::ptr GrpcSDLoadBalance::openStreamClient(const std::string& domain, const std::string& service,
                                 sylar::http::HttpRequest::ptr request, uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return nullptr;
        //return std::make_shared<GrpcResult>(ILoadBalance::NO_SERVICE, "no_service", 0);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        //return std::make_shared<GrpcResult>(ILoadBalance::NO_CONNECTION, "no_connection", 0);
        return nullptr;
    }

    auto cli = conn->getStreamAs<GrpcConnection>()->openStreamClient(request);
    if(cli) {
        return std::make_shared<GrpcStreamClient>(cli);
    }
    return nullptr;
}

}
}
