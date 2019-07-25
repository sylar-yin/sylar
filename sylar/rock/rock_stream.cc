#include "rock_stream.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

RockStream::RockStream(Socket::ptr sock)
    :AsyncSocketStream(sock, true)
    ,m_decoder(new RockMessageDecoder) {
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
        RockSendCtx::ptr ctx(new RockSendCtx);
        ctx->msg = msg;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}

RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms) {
    if(isConnected()) {
        RockCtx::ptr ctx(new RockCtx);
        ctx->request = req;
        ctx->sn = req->getSn();
        ctx->timeout = timeout_ms;
        ctx->scheduler = sylar::Scheduler::GetThis();
        ctx->fiber = sylar::Fiber::GetThis();
        addCtx(ctx);
        ctx->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        sylar::Fiber::YieldToHold();
        return std::make_shared<RockResult>(ctx->result, ctx->response);
    } else {
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, nullptr);
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

RockResult::ptr RockFairLoadBalance::request(RockRequest::ptr req, uint32_t timeout_ms) {
    uint64_t ts = sylar::GetCurrentMS();
    {
        if((ts - m_lastInitTime) > 500) {
            RWMutexType::WriteLock lock(m_mutex);
            initWeight();
            m_lastInitTime = ts;
        }
    }
    auto conn = getAsFair();
    if(!conn) {
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, nullptr);
    }
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
