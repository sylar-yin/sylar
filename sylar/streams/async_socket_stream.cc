#include "async_socket_stream.h"
#include "sylar/util.h"
#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

AsyncSocketStream::Ctx::Ctx()
    :sn(0)
    ,timeout(0)
    ,result(0)
    ,timed(false)
    ,scheduler(nullptr) {
}

void AsyncSocketStream::Ctx::doRsp() {
    Scheduler* scd = scheduler;
    if(!sylar::Atomic::compareAndSwapBool(scheduler, scd, (Scheduler*)nullptr)) {
        return;
    }
    if(!scd || !fiber) {
        return;
    }
    if(timer) {
        timer->cancel();
        timer = nullptr;
    }

    if(timed) {
        result = TIMEOUT;
        resultStr = "timeout";
    }
    scd->schedule(&fiber);
}

AsyncSocketStream::AsyncSocketStream(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner)
    ,m_waitSem(2)
    ,m_sn(0)
    ,m_autoConnect(false)
    ,m_iomanager(nullptr)
    ,m_worker(nullptr) {
}

bool AsyncSocketStream::start() {
    if(!m_iomanager) {
        m_iomanager = sylar::IOManager::GetThis();
    }
    if(!m_worker) {
        m_worker = sylar::IOManager::GetThis();
    }

    do {
        waitFiber();

        if(m_timer) {
            m_timer->cancel();
            m_timer = nullptr;
        }

        if(!isConnected()) {
            if(!m_socket->reconnect()) {
                innerClose();
                m_waitSem.notify();
                m_waitSem.notify();
                break;
            }
        }

        if(m_connectCb) {
            if(!m_connectCb(shared_from_this())) {
                innerClose();
                m_waitSem.notify();
                m_waitSem.notify();
                break;
            }
        }

        startRead();
        startWrite();
        return true;
    } while(false);

    if(m_autoConnect) {
        if(m_timer) {
            m_timer->cancel();
            m_timer = nullptr;
        }

        m_timer = m_iomanager->addTimer(2 * 1000,
                std::bind(&AsyncSocketStream::start, shared_from_this()));
    }
    return false;
}

void AsyncSocketStream::doRead() {
    try {
        while(isConnected()) {
            auto ctx = doRecv();
            if(ctx) {
                ctx->doRsp();
            }
        }
    } catch (...) {
        //TODO log
    }

    SYLAR_LOG_DEBUG(g_logger) << "doRead out " << this;
    innerClose();
    m_waitSem.notify();

    if(m_autoConnect) {
        m_iomanager->addTimer(10, std::bind(&AsyncSocketStream::start, shared_from_this()));
    }
}

void AsyncSocketStream::doWrite() {
    try {
        while(isConnected()) {
            m_sem.wait();
            std::list<SendCtx::ptr> ctxs;
            {
                RWMutexType::WriteLock lock(m_queueMutex);
                m_queue.swap(ctxs);
            }
            auto self = shared_from_this();
            for(auto& i : ctxs) {
                if(!i->doSend(self)) {
                    innerClose();
                    break;
                }
            }
        }
    } catch (...) {
        //TODO log
    }
    SYLAR_LOG_DEBUG(g_logger) << "doWrite out " << this;
    {
        RWMutexType::WriteLock lock(m_queueMutex);
        m_queue.clear();
    }
    m_waitSem.notify();
}

void AsyncSocketStream::startRead() {
    m_iomanager->schedule(std::bind(&AsyncSocketStream::doRead, shared_from_this()));
}

void AsyncSocketStream::startWrite() {
    m_iomanager->schedule(std::bind(&AsyncSocketStream::doWrite, shared_from_this()));
}

void AsyncSocketStream::onTimeOut(Ctx::ptr ctx) {
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_ctxs.erase(ctx->sn);
    }
    ctx->timed = true;
    ctx->doRsp();
}

AsyncSocketStream::Ctx::ptr AsyncSocketStream::getCtx(uint32_t sn) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_ctxs.find(sn);
    return it != m_ctxs.end() ? it->second : nullptr;
}

AsyncSocketStream::Ctx::ptr AsyncSocketStream::getAndDelCtx(uint32_t sn) {
    Ctx::ptr ctx;
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_ctxs.find(sn);
    if(it != m_ctxs.end()) {
        ctx = it->second;
        m_ctxs.erase(it);
    }
    return ctx;
}

bool AsyncSocketStream::addCtx(Ctx::ptr ctx) {
    RWMutexType::WriteLock lock(m_mutex);
    m_ctxs.insert(std::make_pair(ctx->sn, ctx));
    return true;
}

bool AsyncSocketStream::enqueue(SendCtx::ptr ctx) {
    SYLAR_ASSERT(ctx);
    RWMutexType::WriteLock lock(m_queueMutex);
    bool empty = m_queue.empty();
    m_queue.push_back(ctx);
    lock.unlock();
    if(empty) {
        m_sem.notify();
    }
    return empty;
}

bool AsyncSocketStream::innerClose() {
    SYLAR_ASSERT(m_iomanager == sylar::IOManager::GetThis());
    if(isConnected() && m_disconnectCb) {
        m_disconnectCb(shared_from_this());
    }
    SocketStream::close();
    m_sem.notify();
    std::unordered_map<uint32_t, Ctx::ptr> ctxs;
    {
        RWMutexType::WriteLock lock(m_mutex);
        ctxs.swap(m_ctxs);
    }
    {
        RWMutexType::WriteLock lock(m_queueMutex);
        m_queue.clear();
    }
    for(auto& i : ctxs) {
        i.second->result = IO_ERROR;
        i.second->resultStr = "io_error";
        i.second->doRsp();
    }
    return true;
}

bool AsyncSocketStream::waitFiber() {
    m_waitSem.wait();
    m_waitSem.wait();
    return true;
}

void AsyncSocketStream::close() {
    m_autoConnect = false;
    SchedulerSwitcher ss(m_iomanager);
    if(m_timer) {
        m_timer->cancel();
    }
    SocketStream::close();
}

AsyncSocketStreamManager::AsyncSocketStreamManager()
    :m_size(0)
    ,m_idx(0) {
}

void AsyncSocketStreamManager::add(AsyncSocketStream::ptr stream) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas.push_back(stream);
    ++m_size;

    if(m_connectCb) {
        stream->setConnectCb(m_connectCb);
    }

    if(m_disconnectCb) {
        stream->setDisconnectCb(m_disconnectCb);
    }
}

void AsyncSocketStreamManager::clear() {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto& i : m_datas) {
        i->close();
    }
    m_datas.clear();
    m_size = 0;
}
void AsyncSocketStreamManager::setConnection(const std::vector<AsyncSocketStream::ptr>& streams) {
    auto cs = streams;
    RWMutexType::WriteLock lock(m_mutex);
    cs.swap(m_datas);
    m_size = m_datas.size();
    if(m_connectCb || m_disconnectCb) {
        for(auto& i : m_datas) {
            if(m_connectCb) {
                i->setConnectCb(m_connectCb);
            }
            if(m_disconnectCb) {
                i->setDisconnectCb(m_disconnectCb);
            }
        }
    }
    lock.unlock();

    for(auto& i : cs) {
        i->close();
    }
}

AsyncSocketStream::ptr AsyncSocketStreamManager::get() {
    RWMutexType::ReadLock lock(m_mutex);
    for(uint32_t i = 0; i < m_size; ++i) {
        auto idx = sylar::Atomic::addFetch(m_idx, 1);
        if(m_datas[idx % m_size]->isConnected()) {
            return m_datas[idx % m_size];
        }
    }
    return nullptr;
}

void AsyncSocketStreamManager::setConnectCb(connect_callback v) {
    m_connectCb = v;
    RWMutexType::WriteLock lock(m_mutex);
    for(auto& i : m_datas) {
        i->setConnectCb(m_connectCb);
    }
}

void AsyncSocketStreamManager::setDisconnectCb(disconnect_callback v) {
    m_disconnectCb = v;
    RWMutexType::WriteLock lock(m_mutex);
    for(auto& i : m_datas) {
        i->setDisconnectCb(m_disconnectCb);
    }
}

}
