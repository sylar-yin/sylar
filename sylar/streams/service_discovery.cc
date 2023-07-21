#include "service_discovery.h"
#include "sylar/config.h"
#include "sylar/log.h"
#if WITH_REDIS
#include "sylar/db/redis.h"
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sylar/pack/json_encoder.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<uint32_t>::ptr g_service_discover_redis_ttl =
    sylar::Config::Lookup("service_discovery.redis.ttl", (uint32_t)10, "service discovery redis ttl");
static sylar::ConfigVar<uint32_t>::ptr g_service_discover_redis_check =
    sylar::Config::Lookup("service_discovery.redis.check_interval", (uint32_t)3, "service discovery redis check interval");
static sylar::ConfigVar<uint32_t>::ptr g_service_discover_consul_check =
    sylar::Config::Lookup("service_discovery.consul.check_interval", (uint32_t)15, "service discovery consul check interval");

static std::string MapToStr(const std::map<std::string, std::string>& m) {
    std::stringstream ss;
    for(auto it = m.begin();
            it != m.end(); ++it) {
        if(it != m.begin()) {
            ss << "&";
        }
        ss << it->first << "=" << it->second;
    }
    return ss.str();
}

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

    ServiceItemInfo::ptr rt = std::make_shared<ServiceItemInfo>();
    rt->m_id = ((uint64_t)ip_addr << 32) | port;
    rt->m_ip = ip;
    rt->m_port = port;
    rt->m_data = data;

    std::vector<std::string> parts = sylar::split(data, '&');
    for(auto& i : parts) {
        auto pos = i.find('=');
        if(pos == std::string::npos) {
            continue;
        }
        rt->m_datas[i.substr(0, pos)] = i.substr(pos + 1);
    }
    rt->m_updateTime = rt->getDataAs<uint64_t>("update_time");
    rt->m_type = rt->getData("type");
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

std::string ServiceItemInfo::getData(const std::string& key, const std::string& def) const {
    auto it = m_datas.find(key);
    return it == m_datas.end() ? def : it->second;
}

void IServiceDiscovery::addParam(const std::string& key, const std::string& val) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_params[key] = val;
}

std::string IServiceDiscovery::getParam(const std::string& key, const std::string& def) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

void IServiceDiscovery::addServiceCallback(service_callback v) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_cbs.push_back(v);
}

void IServiceDiscovery::setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    //TODO
    //m_queryInfos = v;
    for(auto& i : v) {
        auto& m = m_queryInfos[i.first];
        for(auto& x : i.second) {
            m.insert(x);
        }
    }
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

std::string IServiceDiscovery::toString() {
    std::stringstream ss;
    sylar::RWMutex::ReadLock lock(m_mutex);
    ss << "[Params]" << std::endl;
    for(auto& i : m_params) {
        ss << "\t" << i.first << ":" << i.second << std::endl;
    }
    ss << std::endl;
    ss << "[register_infos]" << std::endl;
    for(auto& i : m_registerInfos) {
        ss << "\t" << i.first << ":" << std::endl;
        for(auto& n : i.second) {
            ss << "\t\t" << n.first << ":" << std::endl;
            for(auto& x : n.second) {
                ss << "\t\t\t" << x.first << ":" << x.second << std::endl;
            }
        }
    }
    ss << std::endl;
    ss << "[query_infos]" << std::endl;
    for(auto& i : m_queryInfos) {
        ss << "\t" << i.first << ":" << std::endl;
        for(auto& n : i.second) {
            ss << "\t\t" << n << std::endl;
        }
    }
    ss << std::endl;
    ss << "[services]" << std::endl;
    for(auto& i : m_datas) {
        ss << "\t" << i.first << ":" << std::endl;
        for(auto& n : i.second) {
            ss << "\t\t" << n.first << ":" << std::endl;
            for(auto& x : n.second) {
                ss << "\t\t\t" << x.first << ": " << x.second->toString() << std::endl;
            }
        }
    }
    lock.unlock();
    return ss.str();
}

#if WITH_ZK_CLIENT
ZKServiceDiscovery::ZKServiceDiscovery(const std::string& hosts)
    :m_hosts(hosts) {
}

bool ZKServiceDiscovery::doRegister() {
    return true;
}

bool ZKServiceDiscovery::doQuery() {
    return true;
}

void ZKServiceDiscovery::start() {
    if(m_client) {
        return;
    }
    auto self = shared_from_this();
    m_client = std::make_shared<sylar::ZKClient>();
    bool b = m_client->init(m_hosts, 6000, std::bind(&ZKServiceDiscovery::onWatch,
                self, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
    if(!b) {
        SYLAR_LOG_ERROR(g_logger) << "ZKClient init fail, hosts=" << m_hosts;
    }
    m_timer = sylar::IOManager::GetThis()->addTimer(60 * 1000, [self, this](){
        m_isOnTimer = true;
        onZKConnect("", m_client);
        m_isOnTimer = false;
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

    ok = true;
    for(auto& i : qinfo) {
        for(auto& x : i.second) {
            ok &= queryInfo(i.first, x);
        }
    }
    if(!ok) {
        SYLAR_LOG_ERROR(g_logger) << "onZKConnect query fail";
    }

    ok = true;
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
    if(!m_isOnTimer) {
        SYLAR_LOG_ERROR(g_logger) << "create path=" << (path + "/" + ip_and_port) << " fail, error:"
            << zerror(rt) << " (" << rt << ")";
    }
    return rt == ZNODEEXISTS;
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
        if(!m_isOnTimer) {
            SYLAR_LOG_ERROR(g_logger) << "create path=" << (path + "/" + m_selfInfo) << " fail, error:"
                << zerror(rt) << " (" << rt << ")";
        }
        return rt == ZNODEEXISTS;
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
    auto cbs = m_cbs;
    lock.unlock();

    for(auto& cb : cbs) {
        cb(domain, service, infos, new_vals);
    }
    return true;
}

bool ZKServiceDiscovery::queryData(const std::string& domain, const std::string& service) {
    //SYLAR_LOG_INFO(g_logger) << "query_data domain=" << domain
    //                         << " service=" << service;
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
    //SYLAR_LOG_INFO(g_logger) << "onZKChild path=" << path;
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
#endif

#if WITH_REDIS
RedisServiceDiscovery::RedisServiceDiscovery(const std::string& name)
    :m_name(name) {
}

bool RedisServiceDiscovery::registerSelf() {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto rinfo = m_registerInfos;
    auto params = m_params;
    lock.unlock();

    std::stringstream hss;
    hss << "hmset sylar";
    for(auto& i : rinfo) {
        std::stringstream ss;
        ss << "hmset sylar:" << i.first;
        hss << " " << i.first << " " << time(0);
        for(auto& n : i.second) {
            ss << " " << n.first << " " << time(0);
            for(auto& v : n.second) {
                std::map<std::string, std::string> m = params;
                if(!v.second.empty()) {
                    m["type"] = v.second;
                }
                m["update_time"] = std::to_string(time(0));
                if(!sylar::RedisUtil::TryCmd(m_name, 5, "hset sylar:%s:%s %s %s",
                        i.first.c_str(), n.first.c_str(), v.first.c_str(), MapToStr(m).c_str())) {
                    SYLAR_LOG_ERROR(g_logger) << "register hset sylar:" << i.first
                        << ":" << n.first << " " << v.first << " " << MapToStr(m) << " fail";
                }
            }
        }
        if(!i.second.empty()) {
            if(!sylar::RedisUtil::TryCmd(m_name, 5, ss.str().c_str())) {
                SYLAR_LOG_ERROR(g_logger) << "register server fail:" << ss.str();
            }
        }
    }
    if(!rinfo.empty()) {
        if(!sylar::RedisUtil::TryCmd(m_name, 5, hss.str().c_str())) {
            SYLAR_LOG_ERROR(g_logger) << "register server fail:" << hss.str();
        }
    }
    return true;
}

bool RedisServiceDiscovery::queryInfo() {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto infos = m_queryInfos;
    lock.unlock();

    time_t now = time(0);
    for(auto& i : infos) {
        auto services = i.second;
        //SYLAR_LOG_INFO(g_logger) << "==" << i.first << " - " << sylar::Join(i.second.begin(), i.second.end(), ",");
        if(i.second.count("all")) {
            auto rpy = sylar::RedisUtil::TryCmd(m_name, 5, "hkeys sylar:%s", i.first.c_str());
            if(!rpy) {
                SYLAR_LOG_ERROR(g_logger) << "query_info error, hkeys sylar:" << i.first;
                continue;
            }
            for(size_t i = 0; i < rpy->elements; ++i) {
                services.insert(std::string(rpy->element[i]->str, rpy->element[i]->len));
            }

            services.erase("all");
        }
        for(auto& n : services) {
            std::unordered_map<uint64_t, ServiceItemInfo::ptr> sinfos;
            auto rpy = sylar::RedisUtil::TryCmd(m_name, 5, "hgetall sylar:%s:%s", i.first.c_str()
                    , n.c_str());
            if(!rpy) {
                SYLAR_LOG_ERROR(g_logger) << "query_info error, hgetall sylar:" << i.first
                    << ":" << n;
                continue;
            }

            for(size_t x = 0; (x + 1) < rpy->elements; x += 2) {
                auto info = ServiceItemInfo::Create(
                        std::string(rpy->element[x]->str, rpy->element[x]->len),
                        std::string(rpy->element[x + 1]->str, rpy->element[x + 1]->len));
                if(!info) {
                    SYLAR_LOG_ERROR(g_logger) << "invalid data format sylar:" << i.first
                        << ":" << n << " " << std::string(rpy->element[x]->str, rpy->element[x]->len);
                    continue;
                }
                //SYLAR_LOG_INFO(g_logger) << "=" << info->toString();
                if(!info || (now - info->getUpdateTime()) > g_service_discover_redis_ttl->getValue()) {
                    //SYLAR_LOG_INFO(g_logger) << "*" << info->toString();
                    continue;
                }
                sinfos[info->getId()] = info;
            }

            auto new_vals = sinfos;
            sylar::RWMutex::WriteLock lock(m_mutex);
            m_datas[i.first][n].swap(sinfos);
            auto cbs = m_cbs;
            lock.unlock();

            for(auto& cb : cbs) {
                cb(i.first, n, sinfos, new_vals);
            }
        }
    }
    return true;
}

void RedisServiceDiscovery::start() {
    //registerSelf();
    queryInfo();

    if(!m_timer) {
        m_timer = sylar::IOManager::GetThis()->addTimer(g_service_discover_redis_check->getValue() * 1000, [this](){
            if(m_startRegister) {
                registerSelf();
            }
            queryInfo();
        }, true);
    }
}

bool RedisServiceDiscovery::doRegister() {
    m_startRegister = true;
    return registerSelf();
}

bool RedisServiceDiscovery::doQuery() {
    return queryInfo();
}

void RedisServiceDiscovery::stop() {
    if(m_timer) {
        m_timer->cancel();
        m_timer = nullptr;
    }
}

#endif

ConsulServiceDiscovery::ConsulServiceDiscovery(const std::string& name, sylar::ConsulClient::ptr client, sylar::ConsulRegisterInfo::ptr regInfo)
    :m_name(name)
    ,m_client(client)
    ,m_regInfo(regInfo) {
}

void ConsulServiceDiscovery::start() {
    queryInfo();

    if(!m_timer) {
        m_timer = sylar::IOManager::GetThis()->addTimer(g_service_discover_consul_check->getValue() * 1000, [this](){
            if(m_startRegister) {
                registerSelf();
            }
            queryInfo();
        }, true);
    }
}

void ConsulServiceDiscovery::stop() {
    if(m_timer) {
        m_timer->cancel();
        m_timer = nullptr;
    }
}

bool ConsulServiceDiscovery::doRegister() {
    m_startRegister = true;
    return registerSelf();
}

bool ConsulServiceDiscovery::doQuery() {
    return queryInfo();
}

bool ConsulServiceDiscovery::registerSelf() {
    bool v = m_client->serviceRegister(m_regInfo);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "consul register fail, "
            << sylar::pack::EncodeToJsonString(m_regInfo, 0);
    }
    return v;
}

bool ConsulServiceDiscovery::queryInfo() {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto infos = m_queryInfos;
    lock.unlock();

    for(auto& i : infos) {
        std::map<std::string, std::list<ConsulServiceInfo::ptr> > cinfos;
        if(!m_client->serviceQuery(i.second, cinfos)) {
            SYLAR_LOG_ERROR(g_logger) << "consul query service fail, " << sylar::Join(i.second.begin(), i.second.end(), ",");
            return false;
        }
        std::unordered_map<uint64_t, ServiceItemInfo::ptr> sinfos;
        for(auto& it : cinfos) {
            for(auto& v : it.second) {
                auto info = ServiceItemInfo::Create(v->ip + ":" + std::to_string(v->port),
                        sylar::MapJoin(v->tags.begin(), v->tags.end()));
                if(!info) {
                    SYLAR_LOG_ERROR(g_logger) << "invalid data format " << it.first
                        << " : " << sylar::pack::EncodeToJsonString(v, 0);
                    continue;
                }
                sinfos[info->getId()] = info;
            }
            auto new_vals = sinfos;
            sylar::RWMutex::WriteLock lock(m_mutex);
            m_datas[i.first][it.first].swap(sinfos);
            auto cbs = m_cbs;
            lock.unlock();

            for(auto& cb : cbs) {
                cb(i.first, it.first, sinfos, new_vals);
            }
        }
    }
    return true;
}

}
