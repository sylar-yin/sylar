#ifndef __SYLAR_STREAMS_SERVICE_DISCOVERY_H__
#define __SYLAR_STREAMS_SERVICE_DISCOVERY_H__

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "sylar/mutex.h"
#include "sylar/iomanager.h"
#include "sylar/zk_client.h"

namespace sylar {

class ServiceItemInfo {
public:
    typedef std::shared_ptr<ServiceItemInfo> ptr;
    static ServiceItemInfo::ptr Create(const std::string& ip_and_port, const std::string& data);

    uint64_t getId() const { return m_id;}
    uint16_t getPort() const { return m_port;}
    const std::string& getIp() const { return m_ip;}
    const std::string& getData() const { return m_data;}

    std::string toString() const;
private:
    uint64_t m_id;
    uint16_t m_port;
    std::string m_ip;
    std::string m_data;
};

class IServiceDiscovery {
public:
    typedef std::shared_ptr<IServiceDiscovery> ptr;
    typedef std::function<void(const std::string& domain, const std::string& service
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value)> service_callback;
    virtual ~IServiceDiscovery() { }

    void registerServer(const std::string& domain, const std::string& service,
                        const std::string& ip_and_port, const std::string& data);
    void queryServer(const std::string& domain, const std::string& service);
    void listServer(std::unordered_map<std::string, std::unordered_map<std::string
                    ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > >& infos);
    void listRegisterServer(std::unordered_map<std::string, std::unordered_map<std::string
                            ,std::unordered_map<std::string, std::string> > >& infos);
    void listQueryServer(std::unordered_map<std::string, std::unordered_set<std::string> >& infos);

    virtual void start() = 0;
    virtual void stop() = 0;

    service_callback getServiceCallback() const { return m_cb;}
    void setServiceCallback(service_callback v) { m_cb = v;}

    void setQueryServer(const std::unordered_map<std::string, std::unordered_set<std::string> >& v);
protected:
    sylar::RWMutex m_mutex;
    //domain -> [service -> [id -> ServiceItemInfo] ]
    std::unordered_map<std::string, std::unordered_map<std::string
        ,std::unordered_map<uint64_t, ServiceItemInfo::ptr> > > m_datas;
    //domain -> [service -> [ip_and_port -> data] ]
    std::unordered_map<std::string, std::unordered_map<std::string
        ,std::unordered_map<std::string, std::string> > > m_registerInfos;
    //domain -> [service]
    std::unordered_map<std::string, std::unordered_set<std::string> > m_queryInfos;

    service_callback m_cb;
};

class ZKServiceDiscovery : public IServiceDiscovery
                          ,public std::enable_shared_from_this<ZKServiceDiscovery> {
public:
    typedef std::shared_ptr<ZKServiceDiscovery> ptr;
    ZKServiceDiscovery(const std::string& hosts);
    const std::string& getSelfInfo() const { return m_selfInfo;}
    void setSelfInfo(const std::string& v) { m_selfInfo = v;}
    const std::string& getSelfData() const { return m_selfData;}
    void setSelfData(const std::string& v) { m_selfData = v;}

    virtual void start();
    virtual void stop();
private:
    void onWatch(int type, int stat, const std::string& path, ZKClient::ptr);
    void onZKConnect(const std::string& path, ZKClient::ptr client);
    void onZKChild(const std::string& path, ZKClient::ptr client);
    void onZKChanged(const std::string& path, ZKClient::ptr client);
    void onZKDeleted(const std::string& path, ZKClient::ptr client);
    void onZKExpiredSession(const std::string& path, ZKClient::ptr client);

    bool registerInfo(const std::string& domain, const std::string& service, 
                      const std::string& ip_and_port, const std::string& data);
    bool queryInfo(const std::string& domain, const std::string& service);
    bool queryData(const std::string& domain, const std::string& service);

    bool existsOrCreate(const std::string& path);
    bool getChildren(const std::string& path);
private:
    std::string m_hosts;
    std::string m_selfInfo;
    std::string m_selfData;
    ZKClient::ptr m_client;
    sylar::Timer::ptr m_timer;
    bool m_isOnTimer = false;
};

}

#endif
