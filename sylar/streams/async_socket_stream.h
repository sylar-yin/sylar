#ifndef __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__
#define __SYLAR_STREAMS_ASYNC_SOCKET_STREAM_H__

#include "socket_stream.h"
#include <list>
#include <unordered_map>
#include <boost/any.hpp>

namespace sylar {

class AsyncSocketStream : public SocketStream
                         ,public std::enable_shared_from_this<AsyncSocketStream> {
public:
    typedef std::shared_ptr<AsyncSocketStream> ptr;
    typedef sylar::RWMutex RWMutexType;
    typedef std::function<bool(AsyncSocketStream::ptr)> connect_callback;
    typedef std::function<void(AsyncSocketStream::ptr)> disconnect_callback;

    AsyncSocketStream(Socket::ptr sock, bool owner = true);

    virtual bool start();
    virtual void close() override;
public:
    enum Error {
        OK = 0,
        TIMEOUT = -1,
        IO_ERROR = -2,
        NOT_CONNECT = -3
    };
protected:
    struct SendCtx {
    public:
        typedef std::shared_ptr<SendCtx> ptr;
        virtual ~SendCtx() {}

        virtual bool doSend(AsyncSocketStream::ptr stream) = 0;
    };

    struct Ctx : public SendCtx {
    public:
        typedef std::shared_ptr<Ctx> ptr;
        virtual ~Ctx() {}
        Ctx();

        uint32_t sn;
        uint32_t timeout;
        uint32_t result;
        bool timed;

        Scheduler* scheduler;
        Fiber::ptr fiber;
        Timer::ptr timer;

        virtual void doRsp();
    };

public:
    void setWorker(sylar::IOManager* v) { m_worker = v;}
    sylar::IOManager* getWorker() const { return m_worker;}

    void setIOManager(sylar::IOManager* v) { m_iomanager = v;}
    sylar::IOManager* getIOManager() const { return m_iomanager;}

    bool isAutoConnect() const { return m_autoConnect;}
    void setAutoConnect(bool v) { m_autoConnect = v;}

    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v) { m_connectCb = v;}
    void setDisconnectCb(disconnect_callback v) { m_disconnectCb = v;}

    template<class T>
    void setData(const T& v) { m_data = v;}

    template<class T>
    T getData() const {
        try {
            return boost::any_cast<T>(m_data);
        } catch (...) {
        }
        return T();
    }
protected:
    virtual void doRead();
    virtual void doWrite();
    virtual void startRead();
    virtual void startWrite();
    virtual void onTimeOut(Ctx::ptr ctx);
    virtual Ctx::ptr doRecv() = 0;

    Ctx::ptr getCtx(uint32_t sn);
    Ctx::ptr getAndDelCtx(uint32_t sn);

    template<class T>
    std::shared_ptr<T> getCtxAs(uint32_t sn) {
        auto ctx = getCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    template<class T>
    std::shared_ptr<T> getAndDelCtxAs(uint32_t sn) {
        auto ctx = getAndDelCtx(sn);
        if(ctx) {
            return std::dynamic_pointer_cast<T>(ctx);
        }
        return nullptr;
    }

    bool addCtx(Ctx::ptr ctx);
    bool enqueue(SendCtx::ptr ctx);

    bool innerClose();
    bool waitFiber();
protected:
    sylar::FiberSemaphore m_sem;
    sylar::FiberSemaphore m_waitSem;
    RWMutexType m_queueMutex;
    std::list<SendCtx::ptr> m_queue;
    RWMutexType m_mutex;
    std::unordered_map<uint32_t, Ctx::ptr> m_ctxs;

    uint32_t m_sn;
    bool m_autoConnect;
    sylar::Timer::ptr m_timer;
    sylar::IOManager* m_iomanager;
    sylar::IOManager* m_worker;

    connect_callback m_connectCb;
    disconnect_callback m_disconnectCb;

    boost::any m_data;
};

class AsyncSocketStreamManager {
public:
    typedef sylar::RWMutex RWMutexType;
    typedef AsyncSocketStream::connect_callback connect_callback;
    typedef AsyncSocketStream::disconnect_callback disconnect_callback;

    AsyncSocketStreamManager();
    virtual ~AsyncSocketStreamManager() {}

    void add(AsyncSocketStream::ptr stream);
    void clear();
    void setConnection(const std::vector<AsyncSocketStream::ptr>& streams);
    AsyncSocketStream::ptr get();
    template<class T>
    std::shared_ptr<T> getAs() {
        auto rt = get();
        if(rt) {
            return std::dynamic_pointer_cast<T>(rt);
        }
        return nullptr;
    }

    connect_callback getConnectCb() const { return m_connectCb;}
    disconnect_callback getDisconnectCb() const { return m_disconnectCb;}
    void setConnectCb(connect_callback v);
    void setDisconnectCb(disconnect_callback v);
private:
    RWMutexType m_mutex;
    uint32_t m_size;
    uint32_t m_idx;
    std::vector<AsyncSocketStream::ptr> m_datas;
    connect_callback m_connectCb;
    disconnect_callback m_disconnectCb;
};

}

#endif
