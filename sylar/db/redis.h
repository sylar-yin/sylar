#ifndef __SYLAR_DB_REDIS_H__
#define __SYLAR_DB_REDIS_H__

#include <stdlib.h>
//#include <hiredis_cluster/hiredis.h>
#include <hiredis_cluster/hircluster.h>
#include <hiredis_cluster/adapters/libevent.h>
#include <sys/time.h>
#include <string>
#include <memory>
#include "sylar/mutex.h"
#include "sylar/db/fox_thread.h"
#include "sylar/singleton.h"

namespace sylar {

typedef std::shared_ptr<redisReply> ReplyPtr;

class IRedis {
public:
    enum Type {
        REDIS = 1,
        REDIS_CLUSTER = 2,
        FOX_REDIS = 3,
        FOX_REDIS_CLUSTER = 4
    };
    typedef std::shared_ptr<IRedis> ptr;
    IRedis() : m_logEnable(true) { }
    virtual ~IRedis() {}

    virtual ReplyPtr cmd(const char* fmt, ...) = 0;
    virtual ReplyPtr cmd(const char* fmt, va_list ap) = 0;
    virtual ReplyPtr cmd(const std::vector<std::string>& argv) = 0;

    const std::string& getName() const { return m_name;}
    void setName(const std::string& v) { m_name = v;}

    const std::string& getPasswd() const { return m_passwd;}
    void setPasswd(const std::string& v) { m_passwd = v;}

    Type getType() const { return m_type;}
protected:
    std::string m_name;
    std::string m_passwd;
    Type m_type;
    bool m_logEnable;
};

class ISyncRedis : public IRedis {
public:
    typedef std::shared_ptr<ISyncRedis> ptr;
    virtual ~ISyncRedis() {}

    virtual bool reconnect() = 0;
    virtual bool connect(const std::string& ip, int port, uint64_t ms = 0) = 0;
    virtual bool connect() = 0;
    virtual bool setTimeout(uint64_t ms) = 0;

    virtual int appendCmd(const char* fmt, ...) = 0;
    virtual int appendCmd(const char* fmt, va_list ap) = 0;
    virtual int appendCmd(const std::vector<std::string>& argv) = 0;

    virtual ReplyPtr getReply() = 0;

    uint64_t getLastActiveTime() const { return m_lastActiveTime;}
    void setLastActiveTime(uint64_t v) { m_lastActiveTime = v;}

protected:
    uint64_t m_lastActiveTime;
};

class Redis : public ISyncRedis {
public:
    typedef std::shared_ptr<Redis> ptr;
    Redis();
    Redis(const std::map<std::string, std::string>& conf);

    virtual bool reconnect();
    virtual bool connect(const std::string& ip, int port, uint64_t ms = 0);
    virtual bool connect();
    virtual bool setTimeout(uint64_t ms);

    virtual ReplyPtr cmd(const char* fmt, ...);
    virtual ReplyPtr cmd(const char* fmt, va_list ap);
    virtual ReplyPtr cmd(const std::vector<std::string>& argv);

    virtual int appendCmd(const char* fmt, ...);
    virtual int appendCmd(const char* fmt, va_list ap);
    virtual int appendCmd(const std::vector<std::string>& argv);

    virtual ReplyPtr getReply();
private:
    std::string m_host;
    uint32_t m_port;
    uint32_t m_connectMs;
    struct timeval m_cmdTimeout;
    std::shared_ptr<redisContext> m_context;
};

class RedisCluster : public ISyncRedis {
public:
    typedef std::shared_ptr<RedisCluster> ptr;
    RedisCluster();
    RedisCluster(const std::map<std::string, std::string>& conf);

    virtual bool reconnect();
    virtual bool connect(const std::string& ip, int port, uint64_t ms = 0);
    virtual bool connect();
    virtual bool setTimeout(uint64_t ms);

    virtual ReplyPtr cmd(const char* fmt, ...);
    virtual ReplyPtr cmd(const char* fmt, va_list ap);
    virtual ReplyPtr cmd(const std::vector<std::string>& argv);

    virtual int appendCmd(const char* fmt, ...);
    virtual int appendCmd(const char* fmt, va_list ap);
    virtual int appendCmd(const std::vector<std::string>& argv);

    virtual ReplyPtr getReply();
private:
    std::string m_host;
    uint32_t m_port;
    uint32_t m_connectMs;
    struct timeval m_cmdTimeout;
    std::shared_ptr<redisClusterContext> m_context;
};

class FoxRedis : public IRedis {
public:
    typedef std::shared_ptr<FoxRedis> ptr;
    enum STATUS {
        UNCONNECTED = 0,
        CONNECTING = 1,
        CONNECTED = 2
    };
    enum RESULT {
        OK = 0,
        TIME_OUT = 1,
        CONNECT_ERR = 2,
        CMD_ERR = 3,
        REPLY_NULL = 4,
        REPLY_ERR = 5,
        INIT_ERR = 6
    };

    FoxRedis(sylar::FoxThread* thr, const std::map<std::string, std::string>& conf);
    ~FoxRedis();

    virtual ReplyPtr cmd(const char* fmt, ...);
    virtual ReplyPtr cmd(const char* fmt, va_list ap);
    virtual ReplyPtr cmd(const std::vector<std::string>& argv);

    bool init();
    int getCtxCount() const { return m_ctxCount;}
private:
    static void OnAuthCb(redisAsyncContext* c, void* rp, void* priv);
private:
    struct FCtx {
        std::string cmd;
        sylar::Scheduler* scheduler;
        sylar::Fiber::ptr fiber;
        ReplyPtr rpy;
    };

    struct Ctx {
        typedef std::shared_ptr<Ctx> ptr;

        event* ev;
        bool timeout;
        FoxRedis* rds;
        std::string cmd;
        FCtx* fctx;
        //std::vector<std::string> parts;
        //sylar::Scheduler* scheduler;
        //sylar::Fiber::ptr fiber;
        //ReplyPtr rpy;
        FoxThread* thread;

        //Ctx::ptr ref;

        Ctx(FoxRedis* rds);
        ~Ctx();
        bool init();
        void cancelEvent();
        static void EventCb(int fd, short event, void* d);
    };
private:
    virtual void pcmd(FCtx* ctx);
    bool pinit();
    void delayDelete(redisAsyncContext* c);
private:
    static void ConnectCb(const redisAsyncContext* c, int status);
    static void DisconnectCb(const redisAsyncContext* c, int status);
    static void CmdCb(redisAsyncContext *c, void *r, void *privdata);
    static void TimeCb(int fd, short event, void* d);
private:
    sylar::FoxThread* m_thread;
    std::shared_ptr<redisAsyncContext> m_context;
    std::string m_host;
    uint16_t m_port;
    STATUS m_status;
    int m_ctxCount;

    struct timeval m_cmdTimeout;
    std::string m_err;
    struct event* m_event;
};

class FoxRedisCluster : public IRedis {
public:
    typedef std::shared_ptr<FoxRedisCluster> ptr;
    enum STATUS {
        UNCONNECTED = 0,
        CONNECTING = 1,
        CONNECTED = 2
    };
    enum RESULT {
        OK = 0,
        TIME_OUT = 1,
        CONNECT_ERR = 2,
        CMD_ERR = 3,
        REPLY_NULL = 4,
        REPLY_ERR = 5,
        INIT_ERR = 6
    };

    FoxRedisCluster(sylar::FoxThread* thr, const std::map<std::string, std::string>& conf);
    ~FoxRedisCluster();

    virtual ReplyPtr cmd(const char* fmt, ...);
    virtual ReplyPtr cmd(const char* fmt, va_list ap);
    virtual ReplyPtr cmd(const std::vector<std::string>& argv);

    int getCtxCount() const { return m_ctxCount;}

    bool init();
private:
    struct FCtx {
        std::string cmd;
        sylar::Scheduler* scheduler;
        sylar::Fiber::ptr fiber;
        ReplyPtr rpy;
    };
    struct Ctx {
        typedef std::shared_ptr<Ctx> ptr;

        event* ev;
        bool timeout;
        FoxRedisCluster* rds;
        FCtx* fctx;
        std::string cmd;
        //std::vector<std::string> parts;
        FoxThread* thread;
        //int cancel_count;
        //int destory;
        //int callback_count;
        //sylar::RWMutex mutex;

        //Ctx::ptr ref;
        //Ctx::ptr tref;
        void cancelEvent();

        Ctx(FoxRedisCluster* rds);
        ~Ctx();
        bool init();
        static void EventCb(int fd, short event, void* d);
    };
private:
    virtual void pcmd(FCtx* ctx);
    bool pinit();
    void delayDelete(redisAsyncContext* c);
    static void OnAuthCb(redisClusterAsyncContext* c, void* rp, void* priv);
private:
    static void ConnectCb(const redisAsyncContext* c, int status);
    static void DisconnectCb(const redisAsyncContext* c, int status);
    static void CmdCb(redisClusterAsyncContext*c, void *r, void *privdata);
    static void TimeCb(int fd, short event, void* d);
private:
    sylar::FoxThread* m_thread;
    std::shared_ptr<redisClusterAsyncContext> m_context;
    std::string m_host;
    STATUS m_status;
    int m_ctxCount;

    struct timeval m_cmdTimeout;
    std::string m_err;
    struct event* m_event;
};

class RedisManager {
public:
    RedisManager();
    IRedis::ptr get(const std::string& name);

    std::ostream& dump(std::ostream& os);
private:
    void freeRedis(IRedis* r);
    void init();
private:
    sylar::RWMutex m_mutex;
    std::map<std::string, std::list<IRedis*> > m_datas;
    std::map<std::string, std::map<std::string, std::string> > m_config;
};

typedef sylar::Singleton<RedisManager> RedisMgr;

class RedisUtil {
public:
    static ReplyPtr Cmd(const std::string& name, const char* fmt, ...);
    static ReplyPtr Cmd(const std::string& name, const char* fmt, va_list ap); 
    static ReplyPtr Cmd(const std::string& name, const std::vector<std::string>& args); 

    static ReplyPtr TryCmd(const std::string& name, uint32_t count, const char* fmt, ...);
    static ReplyPtr TryCmd(const std::string& name, uint32_t count, const std::vector<std::string>& args); 
};

}

#endif
