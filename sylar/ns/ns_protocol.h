#ifndef __SYLAR_NS_NS_PROTOCOL_H__
#define __SYLAR_NS_NS_PROTOCOL_H__

#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <stdint.h>
#include "sylar/mutex.h"
#include "ns_protobuf.pb.h"

namespace sylar {
namespace ns {

enum class NSCommand {
    /// 注册节点信息
    REGISTER        = 0x10001,
    /// 查询节点信息
    QUERY           = 0x10002,
    /// 设置黑名单
    SET_BLACKLIST   = 0x10003,
    /// 查询黑名单
    QUERY_BLACKLIST = 0x10004,
    /// 心跳
    TICK            = 0x10005,
};

enum class NSNotify {
    NODE_CHANGE = 0x10001,
};

class NSNode {
public:
    typedef std::shared_ptr<NSNode> ptr;

    NSNode(const std::string& ip, uint16_t port, uint32_t weight);

    const std::string& getIp() const { return m_ip;}
    uint16_t getPort() const { return m_port;}
    uint32_t getWeight() const { return m_weight;}

    //void setIp(const std::string& v) { m_ip = v;}
    //void setPort(uint16_t v) { m_port = v;}
    void setWeight(uint32_t v) { m_weight = v;}

    uint64_t getId() const { return m_id;}

    static uint64_t GetID(const std::string& ip, uint16_t port);
    std::ostream& dump(std::ostream& os, const std::string& prefix = "");
    std::string toString(const std::string& prefix = "");
private:
    uint64_t m_id;
    std::string m_ip;
    uint16_t m_port;
    uint32_t m_weight;
};

class NSNodeSet {
public:
    typedef std::shared_ptr<NSNodeSet> ptr;
    NSNodeSet(uint32_t cmd);

    void add(NSNode::ptr info);
    NSNode::ptr del(uint64_t id);
    NSNode::ptr get(uint64_t id);

    uint32_t getCmd() const { return m_cmd;}
    void setCmd(uint32_t v) { m_cmd = v;}

    void listAll(std::vector<NSNode::ptr>& infos);
    std::ostream& dump(std::ostream& os, const std::string& prefix = "");
    std::string toString(const std::string& prefix = "");

    size_t size();
private:
    sylar::RWMutex m_mutex;
    uint32_t m_cmd;
    std::map<uint64_t, NSNode::ptr> m_datas;
};

class NSDomain {
public:
    typedef std::shared_ptr<NSDomain> ptr;
    NSDomain(const std::string& domain)
        :m_domain(domain) {
    }

    const std::string& getDomain() const { return m_domain;}
    void setDomain(const std::string& v) { m_domain = v;}

    void add(NSNodeSet::ptr info);
    void add(uint32_t cmd, NSNode::ptr info);
    void del(uint32_t cmd);
    NSNode::ptr del(uint32_t cmd, uint64_t id);
    NSNodeSet::ptr get(uint32_t cmd);
    void listAll(std::vector<NSNodeSet::ptr>& infos);
    std::ostream& dump(std::ostream& os, const std::string& prefix = "");
    std::string toString(const std::string& prefix = "");
    size_t size();
private:
    std::string m_domain;
    sylar::RWMutex m_mutex;
    std::map<uint32_t, NSNodeSet::ptr> m_datas;
};

class NSDomainSet {
public:
    typedef std::shared_ptr<NSDomainSet> ptr;

    void add(NSDomain::ptr info);
    void del(const std::string& domain);
    NSDomain::ptr get(const std::string& domain, bool auto_create = false);

    void del(const std::string& domain, uint32_t cmd, uint64_t id);
    void listAll(std::vector<NSDomain::ptr>& infos);

    std::ostream& dump(std::ostream& os, const std::string& prefix = "");
    std::string toString(const std::string& prefix = "");

    void swap(NSDomainSet& ds);
private:
    sylar::RWMutex m_mutex;
    std::map<std::string, NSDomain::ptr> m_datas;
};

}
}

#endif
