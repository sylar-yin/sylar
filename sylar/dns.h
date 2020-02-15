#ifndef __SYLAR_DNS_H__
#define __SYLAR_DNS_H__

#include "sylar/singleton.h"
#include "sylar/address.h"
#include "sylar/socket.h"
#include "sylar/mutex.h"
#include "sylar/timer.h"
#include <set>

namespace sylar {

class Dns {
public:
    typedef std::shared_ptr<Dns> ptr;
    typedef sylar::RWMutex RWMutexType;
    enum Type {
        TYPE_DOMAIN = 1,
        TYPE_ADDRESS = 2
    };
    //domain: www.sylar.top:80
    Dns(const std::string& domain, int type, uint32_t pool_size = 0);

    //type == 2,
    void set(const std::set<std::string>& addrs);
    sylar::Address::ptr get(uint32_t seed = -1);
    sylar::Socket::ptr getSock(uint32_t seed = -1);

    const std::string& getDomain() const { return m_domain;}
    int getType() const { return m_type;}

    std::string getCheckPath() const { return m_checkPath;}
    void setCheckPath(const std::string& v) { m_checkPath = v;}

    std::string toString();

    void refresh();
public:
    struct AddressItem : public std::enable_shared_from_this<AddressItem> {
        typedef std::shared_ptr<AddressItem> ptr;
        ~AddressItem();
        sylar::Address::ptr addr;
        std::list<Socket*> socks;
        sylar::Spinlock m_mutex;
        bool valid = false;
        uint32_t pool_size = 0;
        std::string check_path;
        
        bool isValid();
        bool checkValid(uint32_t timeout_ms);

        void push(Socket* sock);
        Socket::ptr pop();
        Socket::ptr getSock();
        
        std::string toString();
    };
private:

    void init();

    void initAddress(const std::vector<Address::ptr>& result);
private:
    std::string m_domain;
    int m_type;
    uint32_t m_idx;
    uint32_t m_poolSize = 0;
    std::string m_checkPath;
    RWMutexType m_mutex;
    std::vector<AddressItem::ptr> m_address;
    std::set<std::string> m_addrs;
};

class DnsManager {
public:
    typedef sylar::RWMutex RWMutexType;
    void init();

    void add(Dns::ptr v);
    Dns::ptr get(const std::string& domain);

    //service: www.sylar.top:80
    //cache: 是否需要缓存
    sylar::Address::ptr getAddress(const std::string& service, bool cache, uint32_t seed = -1);
    sylar::Socket::ptr getSocket(const std::string& service, bool cache, uint32_t seed = -1);

    void start();

    std::ostream& dump(std::ostream& os);
private:
    RWMutexType m_mutex;
    std::map<std::string, Dns::ptr> m_dns;
    sylar::Timer::ptr m_timer;
    bool m_refresh = false;
    uint64_t m_lastUpdateTime = 0;
};

typedef sylar::Singleton<DnsManager> DnsMgr;

}

#endif
