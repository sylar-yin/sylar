#ifndef __SYLAR_ZK_CLIENT_H__
#define __SYLAR_ZK_CLIENT_H__

#include <memory>
#include <functional>
#include <string>
#include <stdint.h>
#include <vector>

#ifndef THREADED
#define THREADED
#endif

#include <zookeeper/zookeeper.h>

namespace sylar {

class ZKClient : public std::enable_shared_from_this<ZKClient> {
public:
    class EventType {
    public:
        static const int CREATED; // = ZOO_CREATED_EVENT;
        static const int DELETED; // = ZOO_DELETED_EVENT;
        static const int CHANGED; // = ZOO_CHANGED_EVENT;
        static const int CHILD  ; // = ZOO_CHILD_EVENT;
        static const int SESSION; // = ZOO_SESSION_EVENT;
        static const int NOWATCHING; // = ZOO_NOTWATCHING_EVENT;
    };
    class FlagsType {
    public:
        static const int EPHEMERAL; // = ZOO_EPHEMERAL;
        static const int SEQUENCE;  //  = ZOO_SEQUENCE;
        static const int CONTAINER; // = ZOO_CONTAINER;
    };
    class StateType {
    public:
        static const int EXPIRED_SESSION; // = ZOO_EXPIRED_SESSION_STATE;
        static const int AUTH_FAILED; // = ZOO_AUTH_FAILED_STATE;
        static const int CONNECTING; // = ZOO_CONNECTING_STATE;
        static const int ASSOCIATING; // = ZOO_ASSOCIATING_STATE;
        static const int CONNECTED; // = ZOO_CONNECTED_STATE;
        static const int READONLY; // = ZOO_READONLY_STATE;
        static const int NOTCONNECTED; // = ZOO_NOTCONNECTED_STATE;
    };

    typedef std::shared_ptr<ZKClient> ptr;
    typedef std::function<void(int type, int stat, const std::string& path, ZKClient::ptr)> watcher_callback;
    typedef void(*log_callback)(const char *message);

    ZKClient();
    ~ZKClient();

    bool init(const std::string& hosts, int recv_timeout, watcher_callback cb, log_callback lcb = nullptr);
    int32_t setServers(const std::string& hosts);

    int32_t create(const std::string& path, const std::string& val, std::string& new_path
                   , const struct ACL_vector* acl = &ZOO_OPEN_ACL_UNSAFE
                   , int flags = 0);
    int32_t exists(const std::string& path, bool watch, Stat* stat = nullptr);
    int32_t del(const std::string& path, int version = -1);
    int32_t get(const std::string& path, std::string& val, bool watch, Stat* stat = nullptr);
    int32_t getConfig(std::string& val, bool watch, Stat* stat = nullptr);
    int32_t set(const std::string& path, const std::string& val, int version = -1, Stat* stat = nullptr);
    int32_t getChildren(const std::string& path, std::vector<std::string>& val, bool watch, Stat* stat = nullptr);
    int32_t close();
    int32_t getState();
    std::string  getCurrentServer();

    bool reconnect();
private:
    static void OnWatcher(zhandle_t *zh, int type, int stat, const char *path,void *watcherCtx);
    typedef std::function<void(int type, int stat, const std::string& path)> watcher_callback2;
private:
    zhandle_t* m_handle;
    std::string m_hosts;
    watcher_callback2 m_watcherCb;
    log_callback m_logCb;
    int32_t m_recvTimeout;
};

}

#endif
