#ifndef __SYLAR_NS_NAME_SERVER_MODULE_H__
#define __SYLAR_NS_NAME_SERVER_MODULE_H__

#include "sylar/module.h"
#include "ns_protocol.h"

namespace sylar {
namespace ns {

class NameServerModule;
class NSClientInfo {
friend class NameServerModule;
public:
    typedef std::shared_ptr<NSClientInfo> ptr;
private:
    NSNode::ptr m_node;
    std::map<std::string, std::set<uint32_t> > m_domain2cmds;
};

class NameServerModule : public RockModule {
public:
    typedef std::shared_ptr<NameServerModule> ptr;
    NameServerModule();

    virtual bool handleRockRequest(sylar::RockRequest::ptr request
                        ,sylar::RockResponse::ptr response
                        ,sylar::RockStream::ptr stream) override;
    virtual bool handleRockNotify(sylar::RockNotify::ptr notify
                        ,sylar::RockStream::ptr stream) override;
    virtual bool onConnect(sylar::Stream::ptr stream) override;
    virtual bool onDisconnect(sylar::Stream::ptr stream) override;
    virtual std::string statusString() override;
private:
    bool handleRegister(sylar::RockRequest::ptr request
                        ,sylar::RockResponse::ptr response
                        ,sylar::RockStream::ptr stream);
    bool handleQuery(sylar::RockRequest::ptr request
                        ,sylar::RockResponse::ptr response
                        ,sylar::RockStream::ptr stream);
    bool handleTick(sylar::RockRequest::ptr request
                        ,sylar::RockResponse::ptr response
                        ,sylar::RockStream::ptr stream);

private:
    NSClientInfo::ptr get(sylar::RockStream::ptr rs);
    void set(sylar::RockStream::ptr rs, NSClientInfo::ptr info);

    void setQueryDomain(sylar::RockStream::ptr rs, const std::set<std::string>& ds);

    void doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty);

    std::set<sylar::RockStream::ptr> getStreams(const std::string& domain);
private:
    NSDomainSet::ptr m_domains;

    sylar::RWMutex m_mutex;
    std::map<sylar::RockStream::ptr, NSClientInfo::ptr> m_sessions;

    /// sessoin 关注的域名
    std::map<sylar::RockStream::ptr, std::set<std::string> > m_queryDomains;
    /// 域名对应关注的session
    std::map<std::string, std::set<sylar::RockStream::ptr> > m_domainToSessions;
};

}
}

#endif
