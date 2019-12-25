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
    enum Type {
        TYPE_DOMAIN = 1,
        TYPE_ADDRESS = 2
    };
    //domain: www.sylar.top:80
    Dns(const std::string& domain, int type);

    //type == 2,
    void set(const std::set<std::string>& addrs);
    sylar::Address::ptr get(uint32_t seed = -1);

    const std::string& getDomain() const { return m_domain;}
    int getType() const { return m_type;}

    std::string toString();

    void refresh();
private:
    struct AddressItem {
        sylar::Address::ptr addr;
        bool valid = false;
    };

    void init();
private:
    std::string m_domain;
    int m_type;
    uint32_t m_idx;
    sylar::RWMutex m_mutex;
    std::vector<AddressItem> m_address;
    std::set<std::string> m_addrs;
};

class DnsManager {
public:
    void init();

    void add(Dns::ptr v);
    Dns::ptr get(const std::string& domain);

    //service: www.sylar.top:80
    //cache: 是否需要缓存
    sylar::Address::ptr getAddress(const std::string& service, bool cache, uint32_t seed = -1);

    void start();

    std::ostream& dump(std::ostream& os);
private:
    sylar::RWMutex m_mutex;
    std::map<std::string, Dns::ptr> m_dns;
    sylar::Timer::ptr m_timer;
    bool m_refresh = false;
    uint64_t m_lastUpdateTime = 0;
};

typedef sylar::Singleton<DnsManager> DnsMgr;

}

#endif
