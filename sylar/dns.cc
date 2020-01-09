#include "dns.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/iomanager.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

struct DnsDefine {
    std::string domain;
    int type;
    int pool_size = 0;
    std::set<std::string> addrs;

    bool operator==(const DnsDefine& b) const {
        return domain == b.domain
            && type == b.type
            && addrs == b.addrs;
    }

    bool operator<(const DnsDefine& b) const {
        if(domain != b.domain) {
            return domain < b.domain;
        }
        return false;
    }
};

template<>
class LexicalCast<std::string, DnsDefine> {
public:
    DnsDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        DnsDefine dd;
        if(!n["domain"].IsDefined()) {
            //SYLAR_LOG_ERROR(g_logger) << "dns domain is null, " << n;
            return DnsDefine();
        }
        dd.domain = n["domain"].as<std::string>();
        if(!n["type"].IsDefined()) {
            //SYLAR_LOG_ERROR(g_logger) << "dns type is null, " << n;
            return DnsDefine();
        }
        dd.type = n["type"].as<int>();

        if(n["addrs"].IsDefined()) {
            for(size_t x = 0; x < n["addrs"].size(); ++x) {
                dd.addrs.insert(n["addrs"][x].as<std::string>());
            }
        }

        if(n["pool"].IsDefined()) {
            dd.pool_size = n["pool"].as<int>();
        }
        return dd;
    }
};

template<>
class LexicalCast<DnsDefine, std::string> {
public:
    std::string operator()(const DnsDefine& i) {
        YAML::Node n;
        n["domain"] = i.domain;
        n["type"] = i.type;
        n["pool"] = i.pool_size;
        for(auto& i : i.addrs) {
            n["addrs"].push_back(i);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

static sylar::ConfigVar<std::set<DnsDefine> >::ptr g_dns_defines =
    sylar::Config::Lookup("dns.config", std::set<DnsDefine>(), "dns config");

struct DnsIniter {
    DnsIniter() {
        g_dns_defines->addListener([](const std::set<DnsDefine>& old_value,
                    const std::set<DnsDefine>& new_value){
            for(auto& n : new_value) {
                if(n.type == Dns::TYPE_DOMAIN) {
                    Dns::ptr dns = std::make_shared<Dns>(n.domain, n.type, n.pool_size);
                    dns->refresh();
                    DnsMgr::GetInstance()->add(dns);
                } else if(n.type == Dns::TYPE_ADDRESS) {
                    Dns::ptr dns = std::make_shared<Dns>(n.domain, n.type, n.pool_size);
                    dns->set(n.addrs);
                    DnsMgr::GetInstance()->add(dns);
                } else {
                    SYLAR_LOG_ERROR(g_logger) << "invalid type=" << n.type
                        << " domain=" << n.domain;
                }
            }
        });
    }
};

static DnsIniter __dns_init;

Dns::Dns(const std::string& domain, int type, uint32_t pool_size)
    :m_domain(domain)
    ,m_type(type)
    ,m_idx(0)
    ,m_poolSize(pool_size) {
}

void Dns::set(const std::set<std::string>& addrs) {
    {
        RWMutexType::WriteLock lock(m_mutex);
        m_addrs = addrs;
    }
    init();
}

void Dns::init() {
    if(m_type != TYPE_ADDRESS) {
        SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid type " << m_type;
        return;
    }

    RWMutexType::ReadLock lock2(m_mutex);
    auto addrs = m_addrs;
    lock2.unlock();

    std::vector<Address::ptr> result;
    for(auto& i : addrs) {
        if(!sylar::Address::Lookup(result, i, sylar::Socket::IPv4, sylar::Socket::TCP)) {
            SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid address: " << i;
        }
    }
    initAddress(result);
}

sylar::Address::ptr Dns::get(uint32_t seed) {
    if(seed == (uint32_t)-1) {
        seed = sylar::Atomic::addFetch(m_idx);
    }
    RWMutexType::ReadLock lock(m_mutex);
    for(size_t i = 0; i < m_address.size(); ++i) {
        auto info = m_address[(seed + i) % m_address.size()];
        if(info->valid) {
            return info->addr;
        }
    }
    return nullptr;
}

sylar::Socket::ptr Dns::getSock(uint32_t seed) {
    if(seed == (uint32_t)-1) {
        seed = sylar::Atomic::addFetch(m_idx);
    }
    RWMutexType::ReadLock lock(m_mutex);
    for(size_t i = 0; i < m_address.size(); ++i) {
        auto info = m_address[(seed + i) % m_address.size()];
        auto sock = info->getSock();
        if(sock) {
            return sock;
        }
    }
    return nullptr;
}

std::string Dns::toString() {
    std::stringstream ss;
    ss << "[Dns domain=" << m_domain
       << " type=" << m_type
       << " idx=" << m_idx;
    RWMutexType::ReadLock lock(m_mutex);
    ss << " addrs.size=" << m_address.size() << " addrs=[";
    for(size_t i = 0; i < m_address.size(); ++i) {
        if(i) {
            ss << ",";
        }
        ss << m_address[i]->toString();
    }
    lock.unlock();
    ss << "]";
    return ss.str();
}

bool Dns::AddressItem::isValid() {
    return valid;
}

std::string Dns::AddressItem::toString() {
    std::stringstream ss;
    ss << *addr << ":" << valid;
    if(pool_size > 0) {
        sylar::Spinlock::Lock lock(m_mutex);
        ss << ":" << socks.size();
    }
    return ss.str();
}

bool Dns::AddressItem::checkValid() {
    if(pool_size > 0) {
        std::vector<Socket*> tmp;
        sylar::Spinlock::Lock lock(m_mutex);
        for(auto it = socks.begin();
                it != socks.end();) {
            if((*it)->checkConnected()) {
                return true;
            } else {
                tmp.push_back(*it);
                socks.erase(it++);
            }
        }
        lock.unlock();
        for(auto& i : tmp) {
            delete i;
        }
    }

    sylar::Socket* sock = new sylar::Socket(addr->getFamily(), sylar::Socket::TCP, 0);
    valid = sock->connect(addr, 20);

    if(valid) {
        if(pool_size > 0) {
            sylar::Spinlock::Lock lock(m_mutex);
            socks.push_back(sock);
        } else {
            delete sock;
        }
    } else {
        delete sock;
    }
    return valid;
}

Dns::AddressItem::~AddressItem() {
    for(auto& i : socks) {
        delete i;
    }
}

void Dns::AddressItem::push(Socket* sock) {
    if(sock->checkConnected()) {
        sylar::Spinlock::Lock lock(m_mutex);
        socks.push_back(sock);
    } else {
        delete sock;
    }
}

static void ReleaseSock(Socket* sock, Dns::AddressItem::ptr ai) {
    ai->push(sock);
}

Socket::ptr Dns::AddressItem::pop() {
    if(pool_size == 0) {
        return nullptr;
    }
    sylar::Spinlock::Lock lock(m_mutex);
    if(socks.empty()) {
        return nullptr;
    }
    auto rt = socks.front();
    socks.pop_front();
    lock.unlock();

    Socket::ptr v(rt, std::bind(ReleaseSock, std::placeholders::_1, shared_from_this()));
    return v;
}

Socket::ptr Dns::AddressItem::getSock() {
    if(pool_size > 0) {
        do {
            auto sock = pop();
            if(!sock) {
                break;
            }
            if(sock->checkConnected()) {
                return sock;
            }
        } while(true);
    } else if(valid) {
        sylar::Socket* sock = new sylar::Socket(addr->getFamily(), sylar::Socket::TCP, 0);
        if(sock->connect(addr, 20)) {
            if(pool_size > 0) {
                Socket::ptr v(sock, std::bind(ReleaseSock, std::placeholders::_1, shared_from_this()));
            } else {
                return sylar::Socket::ptr(sock);
            }
        } else {
            delete sock;
        }
    }
    return nullptr;
}

void Dns::initAddress(const std::vector<Address::ptr>& result) {
    std::map<std::string, AddressItem::ptr> old_address;
    {
        RWMutexType::ReadLock lock(m_mutex);
        auto tmp = m_address;
        lock.unlock();

        for(auto& i : tmp) {
            old_address[i->addr->toString()] = i;
        }
    }

    std::vector<AddressItem::ptr> address;
    address.resize(result.size());
    std::map<std::string, AddressItem::ptr> m;
    for(size_t i = 0; i < result.size(); ++i) {
        auto it = old_address.find(result[i]->toString());
        if(it != old_address.end()) {
            it->second->checkValid();
            address[i] = it->second;
            continue;
        }
        auto info = std::make_shared<AddressItem>();
        info->addr = result[i];
        info->pool_size = m_poolSize;
        info->checkValid();
        address[i] = info;
    }

    RWMutexType::WriteLock lock(m_mutex);
    m_address.swap(address);
}

void Dns::refresh() {
    if(m_type == TYPE_DOMAIN) {
        std::vector<Address::ptr> result;
        if(!sylar::Address::Lookup(result, m_domain, sylar::Socket::IPv4, sylar::Socket::TCP)) {
            SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid address: " << m_domain;
        }
        initAddress(result);
    } else {
        init();
    }
}

void DnsManager::add(Dns::ptr v) {
    RWMutexType::WriteLock lock(m_mutex);
    m_dns[v->getDomain()] = v;
}

Dns::ptr DnsManager::get(const std::string& domain) {
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_dns.find(domain);
    return it == m_dns.end() ? nullptr : it->second;
}

sylar::Address::ptr DnsManager::getAddress(const std::string& service, bool cache, uint32_t seed) {
    auto dns = get(service);
    if(dns) {
        return dns->get(seed);
    }

    if(cache) {
        sylar::IOManager::GetThis()->schedule([service, this](){
            Dns::ptr dns = std::make_shared<Dns>(service, Dns::TYPE_DOMAIN);
            dns->refresh();
            add(dns);
        });
    }

    return sylar::Address::LookupAny(service, sylar::Socket::IPv4, sylar::Socket::TCP);
}

sylar::Socket::ptr DnsManager::getSocket(const std::string& service, bool cache, uint32_t seed) {
    auto dns = get(service);
    if(dns) {
        return dns->getSock(seed);
    }

    if(cache) {
        sylar::IOManager::GetThis()->schedule([service, this](){
            Dns::ptr dns = std::make_shared<Dns>(service, Dns::TYPE_DOMAIN, 1);
            dns->refresh();
            add(dns);
        });
    }
    auto addr = sylar::Address::LookupAny(service, sylar::Socket::IPv4, sylar::Socket::TCP);
    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    if(sock->connect(addr, 20)) {
        return sock;
    }
    return nullptr;
}

void DnsManager::init() {
    if(m_refresh) {
        return;
    }
    m_refresh = true;
    RWMutexType::ReadLock lock(m_mutex);
    std::map<std::string, Dns::ptr> dns = m_dns;
    lock.unlock();
    for(auto& i : dns) {
        i.second->refresh();
    }
    m_refresh = false;
    m_lastUpdateTime = time(0);
}

void DnsManager::start() {
    if(m_timer) {
        return;
    }
    m_timer = sylar::IOManager::GetThis()->addTimer(2000, std::bind(&DnsManager::init, this), true);
}

std::ostream& DnsManager::dump(std::ostream& os) {
    RWMutexType::ReadLock lock(m_mutex);
    os << "[DnsManager has_timer=" << (m_timer != nullptr)
       << " last_update_time=" << sylar::Time2Str(m_lastUpdateTime)
       << " dns.size=" << m_dns.size() << "]" << std::endl;
    for(auto& i : m_dns) {
        os << "\t" << i.second->toString() << std::endl;
    }
    return os;
}

}
