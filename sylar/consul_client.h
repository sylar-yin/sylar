#ifndef __SYLAR_CONSUL_CLIENT_H__
#define __SYLAR_CONSUL_CLIENT_H__

#include <memory>
#include <string>
#include <list>
#include <map>
#include <unordered_set>
#include "sylar/pack/pack.h"

namespace sylar {

struct ConsulCheck {
    typedef std::shared_ptr<ConsulCheck> ptr;
    std::string interval;
    std::string http;
    std::map<std::string, std::string> header;
    std::string deregisterCriticalServiceAfter;

    SYLAR_PACK(A("Interval", interval, "HTTP", http, "Header", header, "DeregisterCriticalServiceAfter", deregisterCriticalServiceAfter));
};

struct ConsulRegisterInfo {
    typedef std::shared_ptr<ConsulRegisterInfo> ptr;
    std::string id;
    std::string name;
    std::list<std::string> tags;
    std::string address;
    std::map<std::string, std::string> meta;
    int port;
    ConsulCheck::ptr check;

    SYLAR_PACK(A("ID", id, "Name", name, "Tags", tags, "Address", address, "Meta", meta, "Port", port, "Check", check));
};

struct ConsulServiceInfo {
    typedef std::shared_ptr<ConsulServiceInfo> ptr;
    ConsulServiceInfo(const std::string& _ip, const uint16_t& _port)
        :ip(_ip), port(_port) { }
    std::string ip;
    uint16_t port;

    std::map<std::string, std::string> tags;

    SYLAR_PACK(O(ip, port, tags));
};

class ConsulClient {
public:
    typedef std::shared_ptr<ConsulClient> ptr;

    bool serviceRegister(ConsulRegisterInfo::ptr info);
    bool serviceUnregister(const std::string& id);

    const std::string& getUrl() const { return m_url;}
    void setUrl(const std::string& v) { m_url = v;}

    bool serviceQuery(const std::unordered_set<std::string>& service_names, std::map<std::string, std::list<ConsulServiceInfo::ptr> >& infos);
private:
    std::string m_url;
};

std::string GetConsulUniqID(int port);

}

#endif
