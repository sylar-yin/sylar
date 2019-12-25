#include "dns.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/iomanager.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

struct DnsDefine {
    std::string domain;
    int type;
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
        for(auto& i : i.addrs) {
            n["addrs"].push_back(i);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<DnsDefine> >::ptr g_dns_defines =
    sylar::Config::Lookup("dns.config", std::set<DnsDefine>(), "dns config");

struct DnsIniter {
    DnsIniter() {
        g_dns_defines->addListener([](const std::set<DnsDefine>& old_value,
                    const std::set<DnsDefine>& new_value){
            for(auto& n : new_value) {
                if(n.type == Dns::TYPE_DOMAIN) {
                    Dns::ptr dns(new Dns(n.domain, n.type));
                    dns->refresh();
                    DnsMgr::GetInstance()->add(dns);
                } else if(n.type == Dns::TYPE_ADDRESS) {
                    Dns::ptr dns(new Dns(n.domain, n.type));
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

Dns::Dns(const std::string& domain, int type)
    :m_domain(domain)
    ,m_type(type)
    ,m_idx(0) {
}

static bool check_service_alive(sylar::Address::ptr addr) {
    sylar::Socket::ptr sock = sylar::Socket::CreateTCPSocket();
    return sock->connect(addr, 20);
}

void Dns::set(const std::set<std::string>& addrs) {
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_addrs = addrs;
    }
    init();
}

void Dns::init() {
    if(m_type != TYPE_ADDRESS) {
        SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid type " << m_type;
        return;
    }

    sylar::RWMutex::ReadLock lock2(m_mutex);
    auto addrs = m_addrs;
    lock2.unlock();

    std::vector<Address::ptr> result;
    for(auto& i : addrs) {
        if(!sylar::Address::Lookup(result, i, sylar::Socket::IPv4, sylar::Socket::TCP)) {
            SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid address: " << i;
        }
    }

    std::vector<AddressItem> address;
    address.resize(result.size());
    for(size_t i = 0; i < result.size(); ++i) {
        address[i].addr = result[i];
        address[i].valid = check_service_alive(result[i]);
    }

    sylar::RWMutex::WriteLock lock(m_mutex);
    m_address.swap(address);
}

sylar::Address::ptr Dns::get(uint32_t seed) {
    if(seed == (uint32_t)-1) {
        seed = sylar::Atomic::addFetch(m_idx);
    }
    sylar::RWMutex::ReadLock lock(m_mutex);
    for(size_t i = 0; i < m_address.size(); ++i) {
        auto info = m_address[(seed + i) % m_address.size()];
        if(info.valid) {
            return info.addr;
        }
    }
    return nullptr;
}

std::string Dns::toString() {
    std::stringstream ss;
    ss << "[Dns domain=" << m_domain
       << " type=" << m_type
       << " idx=" << m_idx;
    sylar::RWMutex::ReadLock lock(m_mutex);
    ss << " addrs.size=" << m_address.size() << " addrs=[";
    for(size_t i = 0; i < m_address.size(); ++i) {
        if(i) {
            ss << ",";
        }
        ss << *m_address[i].addr << ":" << m_address[i].valid;
    }
    lock.unlock();
    ss << "]";
    return ss.str();
}

void Dns::refresh() {
    if(m_type == TYPE_DOMAIN) {
        std::vector<Address::ptr> result;
        if(!sylar::Address::Lookup(result, m_domain, sylar::Socket::IPv4, sylar::Socket::TCP)) {
            SYLAR_LOG_ERROR(g_logger) << m_domain << " invalid address: " << m_domain;
        }
        std::vector<AddressItem> address;
        address.resize(result.size());
        for(size_t i = 0; i < result.size(); ++i) {
            address[i].addr = result[i];
            address[i].valid = check_service_alive(result[i]);
        }
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_address.swap(address);
    } else {
        init();
    }
}

void DnsManager::add(Dns::ptr v) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_dns[v->getDomain()] = v;
}

Dns::ptr DnsManager::get(const std::string& domain) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    auto it = m_dns.find(domain);
    return it == m_dns.end() ? nullptr : it->second;
}

sylar::Address::ptr DnsManager::getAddress(const std::string& service, bool cache, uint32_t seed) {
    auto dns = get(service);
    if(dns) {
        return dns->get(seed);
    }

    if(cache) {
        dns.reset(new Dns(service, Dns::TYPE_DOMAIN));
        add(dns);
    }

    return sylar::Address::LookupAny(service, sylar::Socket::IPv4, sylar::Socket::TCP);
}

void DnsManager::init() {
    if(m_refresh) {
        return;
    }
    m_refresh = true;
    sylar::RWMutex::ReadLock lock(m_mutex);
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
    sylar::RWMutex::ReadLock lock(m_mutex);
    os << "[DnsManager has_timer=" << (m_timer != nullptr)
       << " last_update_time=" << sylar::Time2Str(m_lastUpdateTime)
       << " dns.size=" << m_dns.size() << "]" << std::endl;
    for(auto& i : m_dns) {
        os << "\t" << i.second->toString() << std::endl;
    }
    return os;
}

}
