#include "rock_stream.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/worker.h"
#include "sylar/proto/logserver.pb.h"
#include "sylar/module.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> > >::ptr g_rock_services =
    sylar::Config::Lookup("rock_services", std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> >(), "rock_services");

//static sylar::ConfigVar<std::unordered_map<std::string
//    ,std::unordered_map<std::string, std::string> > >::ptr g_rock_services =
//    sylar::Config::Lookup("rock_services", std::unordered_map<std::string
//    ,std::unordered_map<std::string, std::string> >(), "rock_services");

std::string RockResult::toString() const {
    std::stringstream ss;
    ss << "[RockResult result=" << result
       << " result_str=" << resultStr
       << " used=" << used
       << " response=" << (response ? response->toString() : "null")
       << " request=" << (request ? request->toString() : "null")
       << " server=" << server
       << "]";
    return ss.str();
}

RockStream::RockStream(Socket::ptr sock)
    :AsyncSocketStream(sock, true)
    ,m_decoder(std::make_shared<RockMessageDecoder>()) {
    SYLAR_LOG_DEBUG(g_logger) << "RockStream::RockStream "
        << this << " "
        << (sock ? sock->toString() : "");
}

RockStream::~RockStream() {
    SYLAR_LOG_DEBUG(g_logger) << "RockStream::~RockStream "
        << this << " "
        << (m_socket ? m_socket->toString() : "");
}

int32_t RockStream::sendMessage(Message::ptr msg) {
    if(isConnected()) {
        RockSendCtx::ptr ctx = std::make_shared<RockSendCtx>();
        ctx->msg = msg;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}

RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms) {
    if(isConnected()) {
        if(req->getSn() == 0) {
            req->setSn(sylar::Atomic::addFetch(m_sn));
        }
        RockCtx::ptr ctx = std::make_shared<RockCtx>();
        ctx->request = req;
        ctx->sn = req->getSn();
        ctx->timeout = timeout_ms;
        ctx->scheduler = sylar::Scheduler::GetThis();
        ctx->fiber = sylar::Fiber::GetThis();
        addCtx(ctx);
        uint64_t ts = sylar::GetCurrentMS();
        ctx->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        sylar::Fiber::YieldToHold();
        auto rt = std::make_shared<RockResult>(ctx->result, ctx->resultStr, sylar::GetCurrentMS() - ts, ctx->response, req);
        rt->server = getRemoteAddressString();
        return rt;
    } else {
        auto rt = std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, "not_connect " + getRemoteAddressString(), 0, nullptr, req);
        rt->server = getRemoteAddressString();
        return rt;
    }
}

bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)
                ->m_decoder->serializeTo(stream, msg) > 0;
}

bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)
                ->m_decoder->serializeTo(stream, request) > 0;
}

AsyncSocketStream::Ctx::ptr RockStream::doRecv() {
    //SYLAR_LOG_INFO(g_logger) << "doRecv " << this;
    auto msg = m_decoder->parseFrom(shared_from_this());
    if(!msg) {
        innerClose();
        return nullptr;
    }

    int type = msg->getType();
    if(type == Message::RESPONSE) {
        auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
        if(!rsp) {
            SYLAR_LOG_WARN(g_logger) << "RockStream doRecv response not RockResponse: "
                << msg->toString();
            return nullptr;
        }
        RockCtx::ptr ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
        if(!ctx) {
            SYLAR_LOG_WARN(g_logger) << "RockStream request timeout reponse="
                << rsp->toString();
            return nullptr;
        }
        ctx->result = rsp->getResult();
        ctx->resultStr = rsp->getResultStr();
        ctx->response = rsp;
        return ctx;
    } else if(type == Message::REQUEST) {
        auto req = std::dynamic_pointer_cast<RockRequest>(msg);
        if(!req) {
            SYLAR_LOG_WARN(g_logger) << "RockStream doRecv request not RockRequest: "
                << msg->toString();
            return nullptr;
        }
        if(m_requestHandler) {
            m_worker->schedule(std::bind(&RockStream::handleRequest,
                        std::dynamic_pointer_cast<RockStream>(shared_from_this()),
                        req));
        } else {
            SYLAR_LOG_WARN(g_logger) << "unhandle request " << req->toString();
        }
    } else if(type == Message::NOTIFY) {
        auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
        if(!nty) {
            SYLAR_LOG_WARN(g_logger) << "RockStream doRecv notify not RockNotify: "
                << msg->toString();
            return nullptr;
        }

        if(m_notifyHandler) {
            m_worker->schedule(std::bind(&RockStream::handleNotify,
                        std::dynamic_pointer_cast<RockStream>(shared_from_this()),
                        nty));
        } else {
            SYLAR_LOG_WARN(g_logger) << "unhandle notify " << nty->toString();
        }
    } else {
        SYLAR_LOG_WARN(g_logger) << "RockStream recv unknow type=" << type
            << " msg: " << msg->toString();
    }
    return nullptr;
}

void RockStream::handleRequest(sylar::RockRequest::ptr req) {
    sylar::RockResponse::ptr rsp = req->createResponse();
    if(!m_requestHandler(req, rsp
        ,std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        sendMessage(rsp);
        //innerClose();
        close();
    } else {
        sendMessage(rsp);
    }
}

void RockStream::handleNotify(sylar::RockNotify::ptr nty) {
    if(!m_notifyHandler(nty
        ,std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        //innerClose();
        close();
    }
}

RockSession::RockSession(Socket::ptr sock)
    :RockStream(sock) {
    m_autoConnect = false;
}

RockConnection::RockConnection()
    :RockStream(nullptr) {
    m_autoConnect = true;
}

bool RockConnection::connect(sylar::Address::ptr addr) {
    m_socket = sylar::Socket::CreateTCP(addr);
    return m_socket->connect(addr);
}

RockSDLoadBalance::RockSDLoadBalance(IServiceDiscovery::ptr sd)
    :SDLoadBalance(sd) {
    m_type = "rock";
}

static SocketStream::ptr create_rock_stream(const std::string& domain, const std::string& service, ServiceItemInfo::ptr info) {
    //SYLAR_LOG_INFO(g_logger) << "create_rock_stream: " << info->toString();
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress(info->getIp());
    if(!addr) {
        SYLAR_LOG_ERROR(g_logger) << "invalid service info: " << info->toString();
        return nullptr;
    }
    addr->setPort(info->getPort());

    RockConnection::ptr conn = std::make_shared<RockConnection>();

    sylar::WorkerMgr::GetInstance()->schedule("service_io", [conn, addr, domain, service](){
        if(domain == "logserver") {
            conn->setConnectCb([conn](sylar::AsyncSocketStream::ptr){
                sylar::RockRequest::ptr req = std::make_shared<sylar::RockRequest>();
                req->setCmd(100);
                logserver::LoginRequest login_req;
                login_req.set_ipport(sylar::Module::GetServiceIPPort("rock"));
                login_req.set_host(sylar::GetHostName());
                login_req.set_pid(getpid());
                req->setAsPB(login_req);
                auto r = conn->request(req, 200);
                if(r->result) {
                    SYLAR_LOG_ERROR(g_logger) << "logserver login error: " << r->toString();
                    return false;
                }
                return true;
            });
        }
        conn->connect(addr);
        conn->start();
    });
    return conn;
}

void RockSDLoadBalance::start() {
    m_cb = create_rock_stream;
    initConf(g_rock_services->getValue());
    SDLoadBalance::start();
}

void RockSDLoadBalance::start(const std::unordered_map<std::string
                              ,std::unordered_map<std::string,std::string> >& confs) {
    m_cb = create_rock_stream;
    initConf(confs);
    SDLoadBalance::start();
}

void RockSDLoadBalance::stop() {
    SDLoadBalance::stop();
}

RockResult::ptr RockSDLoadBalance::request(const std::string& domain, const std::string& service,
                                           RockRequest::ptr req, uint32_t timeout_ms, uint64_t idx) {
    auto lb = get(domain, service);
    if(!lb) {
        return std::make_shared<RockResult>(ILoadBalance::NO_SERVICE, "no_service", 0, nullptr, req);
    }
    auto conn = lb->get(idx);
    if(!conn) {
        return std::make_shared<RockResult>(ILoadBalance::NO_CONNECTION, "no_connection", 0, nullptr, req);
    }
    uint64_t ts = sylar::GetCurrentMS();
    auto& stats = conn->get(ts / 1000);
    stats.incDoing(1);
    stats.incTotal(1);
    auto r = conn->getStreamAs<RockStream>()->request(req, timeout_ms);
    uint64_t ts2 = sylar::GetCurrentMS();
    if(r->result == 0) {
        stats.incOks(1);
        stats.incUsedTime(ts2 -ts);
    } else if(r->result == AsyncSocketStream::TIMEOUT) {
        stats.incTimeouts(1);
    } else if(r->result < 0) {
        stats.incErrs(1);
    }
    stats.decDoing(1);
    return r;
}

}
