#include "service_discovery.h"
#include "sylar/log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ServiceItemInfo::ptr ServiceItemInfo::Create(const std::string& ip_and_port, const std::string& data) {
    auto pos = ip_and_port.find(':');
    if(pos == std::string::npos) {
        return nullptr;
    }
    auto ip = ip_and_port.substr(0, pos);
    auto port = sylar::TypeUtil::Atoi(ip_and_port.substr(pos + 1));
    in_addr_t ip_addr = inet_addr(ip.c_str());
    if(ip_addr == 0) {
        return nullptr;
    }

    ServiceItemInfo::ptr rt(new ServiceItemInfo);
    rt->m_id = ((uint64_t)ip_addr << 32) | port;
    rt->m_ip = ip;
    rt->m_port = port;
    rt->m_data = data;
    return rt;
}

std::string ServiceItemInfo::toString() const {
    std::stringstream ss;
    ss << "[ServiceItemInfo id=" << m_id
       << " ip=" << m_ip
       << " port=" << m_port
       << " data=" << m_data
       << "]";
    return ss.str();
}

void IServiceDiscovery::setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_queryInfos = v;
}

void IServiceDiscovery::registerServer(const std::string& domain, const std::string& service,
                                       const std::string& ip_and_port, const std::string& data) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_registerInfos[domain][service][ip_and_port] = data;
}

void IServiceDiscovery::queryServer(const std::string& domain, const std::string& service) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_queryInfos[domain].insert(service);
}

void IServiceDiscovery::listServer(std::unordered_map<std::string, std::unordered_map<std::string
                                   ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > >& infos) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    infos = m_datas;
}

void IServiceDiscovery::listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string
                                           ,std::unordered_map<std::string, std::string> > >& infos) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    infos = m_registerInfos;
}

void IServiceDiscovery::listQueryServer(std::unordered_map<std::string
                                        ,std::unordered_set<std::string> >& infos) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    infos = m_queryInfos;
}

ZKServiceDiscovery::ZKServiceDiscovery(const std::string& hosts)
    :m_hosts(hosts) {
}

void ZKServiceDiscovery::start() {
    if(m_client) {
        return;
    }
    auto self = shared_from_this();
    m_client.reset(new sylar::ZKClient);
    bool b = m_client->init(m_hosts, 6000, std::bind(&ZKServiceDiscovery::onWatch,
                self, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
    if(!b) {
        SYLAR_LOG_ERROR(g_logger) << "ZKClient init fail, hosts=" << m_hosts;
    }
    m_timer = sylar::IOManager::GetThis()->addTimer(60 * 1000, [self, this](){
        sylar::RWMutex::ReadLock lock(m_mutex);
        auto qinfo = m_queryInfos;
        lock.unlock();

        for(auto& i : qinfo) {
            for(auto& x : i.second) {
                queryData(i.first, x);
            }
        }
    }, true);
}

void ZKServiceDiscovery::stop() {
    if(m_client) {
        m_client->close();
        m_client = nullptr;
    }
    if(m_timer) {
        m_timer->cancel();
        m_timer = nullptr;
    }
}

void ZKServiceDiscovery::onZKConnect(const std::string& path, ZKClient::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "onZKConnect path=" << path;
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto rinfo = m_registerInfos;
    auto qinfo = m_queryInfos;
    lock.unlock();

    bool ok = true;
    for(auto& i : rinfo) {
        for(auto& x : i.second) {
            for(auto& v : x.second) {
                ok &= registerInfo(i.first, x.first, v.first, v.second);
            }
        }
    }

    if(!ok) {
        SYLAR_LOG_ERROR(g_logger) << "onZKConnect register fail";
    }

    for(auto& i : qinfo) {
        for(auto& x : i.second) {
            ok &= queryInfo(i.first, x);
        }
    }
    if(!ok) {
        SYLAR_LOG_ERROR(g_logger) << "onZKConnect query fail";
    }

    for(auto& i : qinfo) {
        for(auto& x : i.second) {
            ok &= queryData(i.first, x);
        }
    }

    if(!ok) {
        SYLAR_LOG_ERROR(g_logger) << "onZKConnect queryData fail";
    }
}

bool ZKServiceDiscovery::existsOrCreate(const std::string& path) {
    int32_t v = m_client->exists(path, false);
    if(v == ZOK) {
        return true;
    } else {
        auto pos = path.find_last_of('/');
        if(pos == std::string::npos) {
            SYLAR_LOG_ERROR(g_logger) << "existsOrCreate invalid path=" << path;
            return false;
        }
        if(pos == 0 || existsOrCreate(path.substr(0, pos))) {
            std::string new_val(1024, 0);
            v = m_client->create(path, "", new_val);
            if(v != ZOK) {
                SYLAR_LOG_ERROR(g_logger) << "create path=" << path << " error:"
                    << zerror(v) << " (" << v << ")";
                return false;
            }
            return true;
        }
        //if(pos == 0) {
        //    std::string new_val(1024, 0);
        //    if(m_client->create(path, "", new_val) != ZOK) {
        //        return false;
        //    }
        //    return true;
        //}
    }
    return false;
}

static std::string GetProvidersPath(const std::string& domain, const std::string& service) {
    return "/sylar/" + domain + "/" + service + "/providers";
}

static std::string GetConsumersPath(const std::string& domain, const std::string& service) {
    return "/sylar/" + domain + "/" + service + "/consumers";
}

static std::string GetDomainPath(const std::string& domain) {
    return "/sylar/" + domain;
}

bool ParseDomainService(const std::string& path, std::string& domain, std::string& service) {
    auto v = sylar::split(path, '/');
    if(v.size() != 5) {
        return false;
    }
    domain = v[2];
    service = v[3];
    return true;
}

bool ZKServiceDiscovery::registerInfo(const std::string& domain, const std::string& service, 
                                      const std::string& ip_and_port, const std::string& data) {
    std::string path = GetProvidersPath(domain, service);
    bool v = existsOrCreate(path);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "create path=" << path << " fail";
        return false;
    }

    std::string new_val(1024, 0);
    int32_t rt = m_client->create(path + "/" + ip_and_port, data, new_val
                                  ,&ZOO_OPEN_ACL_UNSAFE, ZKClient::FlagsType::EPHEMERAL);
    if(rt == ZOK) {
        return true;
    }
    SYLAR_LOG_ERROR(g_logger) << "create path=" << (path + "/" + ip_and_port) << " fail, error:"
        << zerror(rt) << " (" << rt << ")";
    return false;
}

bool ZKServiceDiscovery::queryInfo(const std::string& domain, const std::string& service) {
    if(service != "all") {
        std::string path = GetConsumersPath(domain, service);
        bool v = existsOrCreate(path);
        if(!v) {
            SYLAR_LOG_ERROR(g_logger) << "create path=" << path << " fail";
            return false;
        }

        if(m_selfInfo.empty()) {
            SYLAR_LOG_ERROR(g_logger) << "queryInfo selfInfo is null";
            return false;
        }

        std::string new_val(1024, 0);
        int32_t rt = m_client->create(path + "/" + m_selfInfo, m_selfData, new_val
                                      ,&ZOO_OPEN_ACL_UNSAFE, ZKClient::FlagsType::EPHEMERAL);
        if(rt == ZOK) {
            return true;
        }
        SYLAR_LOG_ERROR(g_logger) << "create path=" << (path + "/" + m_selfInfo) << " fail, error:"
            << zerror(rt) << " (" << rt << ")";
        return false;
    } else {
        std::vector<std::string> children;
        m_client->getChildren(GetDomainPath(domain), children, false);
        bool rt = true;
        for(auto& i : children) {
            rt &= queryInfo(domain, i);
        }
        return rt;
    }
}

bool ZKServiceDiscovery::getChildren(const std::string& path) {
    std::string domain;
    std::string service;
    if(!ParseDomainService(path, domain, service)) {
        SYLAR_LOG_ERROR(g_logger) << "get_children path=" << path
            << " invalid path";
        return false;
    }
    {
        sylar::RWMutex::ReadLock lock(m_mutex);
        auto it = m_queryInfos.find(domain);
        if(it == m_queryInfos.end()) {
            SYLAR_LOG_ERROR(g_logger) << "get_children path=" << path
                << " domian=" << domain << " not exists";
            return false;
        }
        if(it->second.count(service) == 0
                && it->second.count("all") == 0) {
            SYLAR_LOG_ERROR(g_logger) << "get_children path=" << path
                << " service=" << service << " not exists "
                << sylar::Join(it->second.begin(), it->second.end(), ",");
            return false;
        }
    }

    std::vector<std::string> vals;
    int32_t v = m_client->getChildren(path, vals, true);
    if(v != ZOK) {
        SYLAR_LOG_ERROR(g_logger) << "get_children path=" << path << " fail, error:"
            << zerror(v) << " (" << v << ")";
        return false;
    }
    std::unordered_map<uint64_t, ServiceItemInfo::ptr> infos;
    for(auto& i : vals) {
        auto info = ServiceItemInfo::Create(i, "");
        if(!info) {
            continue;
        }
        infos[info->getId()] = info;
        SYLAR_LOG_INFO(g_logger) << "domain=" << domain
            << " service=" << service << " info=" << info->toString();
    }

    auto new_vals = infos;
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_datas[domain][service].swap(infos);
    lock.unlock();

    m_cb(domain, service, infos, new_vals);
    return true;
}

bool ZKServiceDiscovery::queryData(const std::string& domain, const std::string& service) {
    if(service != "all") {
        std::string path = GetProvidersPath(domain, service);
        return getChildren(path);
    } else {
        std::vector<std::string> children;
        m_client->getChildren(GetDomainPath(domain), children, false);
        bool rt = true;
        for(auto& i : children) {
            rt &= queryData(domain, i);
        }
        return rt;
    }
}

void ZKServiceDiscovery::onZKChild(const std::string& path, ZKClient::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "onZKChild path=" << path;
    getChildren(path);
}

void ZKServiceDiscovery::onZKChanged(const std::string& path, ZKClient::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "onZKChanged path=" << path;
}

void ZKServiceDiscovery::onZKDeleted(const std::string& path, ZKClient::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "onZKDeleted path=" << path;
}

void ZKServiceDiscovery::onZKExpiredSession(const std::string& path, ZKClient::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "onZKExpiredSession path=" << path;
    client->reconnect();
}

void ZKServiceDiscovery::onWatch(int type, int stat, const std::string& path, ZKClient::ptr client) {
    if(stat == ZKClient::StateType::CONNECTED) {
        if(type == ZKClient::EventType::SESSION) {
                return onZKConnect(path, client);
        } else if(type == ZKClient::EventType::CHILD) {
                return onZKChild(path, client);
        } else if(type == ZKClient::EventType::CHANGED) {
                return onZKChanged(path, client);
        } else if(type == ZKClient::EventType::DELETED) {
                return onZKDeleted(path, client);
        } 
    } else if(stat == ZKClient::StateType::EXPIRED_SESSION) {
        if(type == ZKClient::EventType::SESSION) {
            return onZKExpiredSession(path, client);
        }
    }
    SYLAR_LOG_ERROR(g_logger) << "onWatch hosts=" << m_hosts
        << " type=" << type << " stat=" << stat
        << " path=" << path << " client=" << client;
}

}
