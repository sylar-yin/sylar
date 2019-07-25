#include "name_server_module.h"
#include "sylar/log.h"
#include "sylar/worker.h"

namespace sylar {
namespace ns {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

uint64_t s_request_count = 0;
uint64_t s_on_connect = 0;
uint64_t s_on_disconnect = 0;

NameServerModule::NameServerModule()
    :RockModule("NameServerModule", "1.0.0", "") {
    m_domains = std::make_shared<NSDomainSet>();
}

bool NameServerModule::handleRockRequest(sylar::RockRequest::ptr request
                                         ,sylar::RockResponse::ptr response
                                         ,sylar::RockStream::ptr stream) {
    sylar::Atomic::addFetch(s_request_count, 1);
    switch(request->getCmd()) {
        case (int)NSCommand::REGISTER:
            return handleRegister(request, response, stream);
        case (int)NSCommand::QUERY:
            return handleQuery(request, response, stream);
        case (int)NSCommand::TICK:
            return handleTick(request, response, stream);
        default:
            SYLAR_LOG_WARN(g_logger) << "invalid cmd=0x" << std::hex << request->getCmd();
            break;
    }
    return true;
}

bool NameServerModule::handleRockNotify(sylar::RockNotify::ptr notify
                                        ,sylar::RockStream::ptr stream) {
    return true;
}

bool NameServerModule::onConnect(sylar::Stream::ptr stream) {
    sylar::Atomic::addFetch(s_on_connect, 1);
    auto rockstream = std::dynamic_pointer_cast<RockStream>(stream);
    if(!rockstream) {
        SYLAR_LOG_ERROR(g_logger) << "invalid stream";
        return false;
    }
    auto addr = rockstream->getRemoteAddress();
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << "onConnect: " << *addr;
    }
    return true;
}

bool NameServerModule::onDisconnect(sylar::Stream::ptr stream) {
    sylar::Atomic::addFetch(s_on_disconnect, 1);
    auto rockstream = std::dynamic_pointer_cast<RockStream>(stream);
    if(!rockstream) {
        SYLAR_LOG_ERROR(g_logger) << "invalid stream";
        return false;
    }
    auto addr = rockstream->getRemoteAddress();
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << "onDisconnect: " << *addr;
    }
    set(rockstream, nullptr);
    //setQueryDomain(rockstream, {});
    return true;
}

NSClientInfo::ptr NameServerModule::get(sylar::RockStream::ptr rs) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_sessions.find(rs);
    return it == m_sessions.end() ? nullptr : it->second;
}

bool NameServerModule::handleRegister(sylar::RockRequest::ptr request
                                      ,sylar::RockResponse::ptr response
                                      ,sylar::RockStream::ptr stream) {
    auto rr = request->getAsPB<RegisterRequest>();
    if(!rr) {
        SYLAR_LOG_ERROR(g_logger) << "invalid register request from: "
            << stream->getRemoteAddressString();
        return false;
    }
    auto old_value = get(stream);
    NSClientInfo::ptr new_value;
    for(int i = 0; i < rr->infos_size(); ++i) {
        auto& info = rr->infos(i);
#define XX(info, attr) \
        if(!info.has_##attr()) { \
            SYLAR_LOG_ERROR(g_logger) << "invalid register request from: " \
                << stream->getRemoteAddressString() \
                << " " #attr " is null"; \
            return false; \
        }
        XX(info, node);
        XX(info, domain);

        if(info.cmds_size() == 0) {
            SYLAR_LOG_ERROR(g_logger) << "invalid register request from: "
                << stream->getRemoteAddressString()
                << " cmds is null";
            return false;
        }
        auto& node = info.node();
        XX(node, ip);
        XX(node, port);
        XX(node, weight);

        NSNode::ptr ns_node(new NSNode(node.ip(), node.port(), node.weight()));
        if(!(ns_node->getId() >> 32)) {
            SYLAR_LOG_ERROR(g_logger) << "invalid register request from: "
                << stream->getRemoteAddressString()
                << " ip=" << node.ip() << " invalid";
            return false;
        }

        if(old_value) {
            if(old_value->m_node->getId() != ns_node->getId()) {
                SYLAR_LOG_ERROR(g_logger) << "invalid register request from: "
                    << stream->getRemoteAddressString()
                    << " old.ip=" << old_value->m_node->getIp()
                    << " old.port=" << old_value->m_node->getPort()
                    << " cur.ip=" << ns_node->getIp()
                    << " cur.port=" << ns_node->getPort();
                return false;
            }
        }
        if(new_value) {
            if(new_value->m_node->getId() != ns_node->getId()) {
                SYLAR_LOG_ERROR(g_logger) << "invalid register request from: "
                    << stream->getRemoteAddressString()
                    << " new.ip=" << new_value->m_node->getIp()
                    << " new.port=" << new_value->m_node->getPort()
                    << " cur.ip=" << ns_node->getIp()
                    << " cur.port=" << ns_node->getPort();
                return false;
            }
        } else {
            new_value.reset(new NSClientInfo);
            new_value->m_node = ns_node;
        }
        for(auto& cmd : info.cmds()) {
            new_value->m_domain2cmds[info.domain()].insert(cmd);
        }
    }
    set(stream, new_value);
    response->setResult(0);
    response->setResultStr("ok");
    return true;
}

void diff(const std::map<std::string, std::set<uint32_t> >& old_value,
          const std::map<std::string, std::set<uint32_t> >& new_value,
          std::map<std::string, std::set<uint32_t> >& dels,
          std::map<std::string, std::set<uint32_t> >& news,
          std::map<std::string, std::set<uint32_t> >& comms) {
    for(auto& i : old_value) {
        auto it = new_value.find(i.first);
        if(it == new_value.end()) {
            dels.insert(i);
            continue;
        }
        for(auto& n : i.second) {
            auto iit = it->second.find(n);
            if(iit == it->second.end()) {
                dels[i.first].insert(n);
                continue;
            }
            comms[i.first].insert(n);
        }
    }

    for(auto& i : new_value) {
        auto it = old_value.find(i.first);
        if(it == old_value.end()) {
            news.insert(i);
            continue;
        }
        for(auto& n : i.second) {
            auto iit = it->second.find(n);
            if(iit == it->second.end()) {
                news[i.first].insert(n);
                continue;
            }
        }
    }
}


void NameServerModule::set(sylar::RockStream::ptr rs, NSClientInfo::ptr new_value) {
    if(!rs->isConnected()) {
        new_value = nullptr;
    }

    auto old_value = get(rs);

    std::map<std::string, std::set<uint32_t> > old_v;
    std::map<std::string, std::set<uint32_t> > new_v;
    std::map<std::string, std::set<uint32_t> > dels;
    std::map<std::string, std::set<uint32_t> > news;
    std::map<std::string, std::set<uint32_t> > comms;

    //auto nty = std::make_shared<NotifyMessage>();
    //std::set<std::string> ds;

    if(old_value) {
        old_v = old_value->m_domain2cmds;
    }
    if(new_value) {
        new_v = new_value->m_domain2cmds;
    }
    diff(old_v, new_v, dels, news, comms);
    for(auto& i : dels) {
        auto d = m_domains->get(i.first);
        if(d) {
            for(auto& c : i.second) {
                d->del(c, old_value->m_node->getId());
                //auto info = nty->add_dels();
                //info->set_domain(i.first);
                //info->set_cmd(c);
                //auto ninfo = info->add_nodes();
                //ninfo->set_ip(old_value->m_node->getIp());
                //ninfo->set_port(old_value->m_node->getPort());
                //ninfo->set_weight(old_value->m_node->getWeight());
            }
            //ds.insert(i.first);
        }
    }
    for(auto& i : news) {
        auto d = m_domains->get(i.first);
        if(!d) {
            d.reset(new NSDomain(i.first));
            m_domains->add(d);
        }
        for(auto& c : i.second) {
            d->add(c, new_value->m_node);

            //auto info = nty->add_updates();
            //info->set_domain(i.first);
            //info->set_cmd(c);
            //auto ninfo = info->add_nodes();
            //ninfo->set_ip(new_value->m_node->getIp());
            //ninfo->set_port(new_value->m_node->getPort());
            //ninfo->set_weight(new_value->m_node->getWeight());
        }
        //ds.insert(i.first);
    }
    if(!comms.empty()) {
        if(old_value->m_node->getWeight() != new_value->m_node->getWeight()) {
            for(auto& i : comms) {
                auto d = m_domains->get(i.first);
                if(!d) {
                    d.reset(new NSDomain(i.first));
                    m_domains->add(d);
                }
                for(auto& c : i.second) {
                    d->add(c, new_value->m_node);

                    //auto info = nty->add_updates();
                    //info->set_domain(i.first);
                    //info->set_cmd(c);
                    //auto ninfo = info->add_nodes();
                    //ninfo->set_ip(new_value->m_node->getIp());
                    //ninfo->set_port(new_value->m_node->getPort());
                    //ninfo->set_weight(new_value->m_node->getWeight());
                }

                //ds.insert(i.first);
            }
        }
    }

    //sylar::WorkerMgr::GetInstance()->schedule("notify",
    //        std::bind(&NameServerModule::doNotify, this, ds, nty));

    sylar::RWMutex::WriteLock lock(m_mutex);
    if(new_value) {
        m_sessions[rs] = new_value;
    } else {
        m_sessions.erase(rs);
    }
}

std::set<sylar::RockStream::ptr> NameServerModule::getStreams(const std::string& domain) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_domainToSessions.find(domain);
    return it == m_domainToSessions.end() ? std::set<sylar::RockStream::ptr>() : it->second;
}

void NameServerModule::doNotify(std::set<std::string>& domains, std::shared_ptr<NotifyMessage> nty) {
    RockNotify::ptr notify(new RockNotify());
    notify->setNotify((int)NSNotify::NODE_CHANGE);
    notify->setAsPB(*nty);
    for(auto& i : domains) {
        auto ss = getStreams(i);
        for(auto& n : ss) {
            n->sendMessage(notify);
        }
    }
}

bool NameServerModule::handleQuery(sylar::RockRequest::ptr request
                                   ,sylar::RockResponse::ptr response
                                   ,sylar::RockStream::ptr stream) {
    auto qreq = request->getAsPB<QueryRequest>();
    if(!qreq) {
        SYLAR_LOG_ERROR(g_logger) << "invalid query request from: "
            << stream->getRemoteAddressString();
        return false;
    }
    if(!qreq->domains_size()) {
        SYLAR_LOG_ERROR(g_logger) << "invalid query request from: "
            << stream->getRemoteAddressString()
            << " domains is null";
    }
    std::set<NSDomain::ptr> domains;
    std::set<std::string> ds;
    for(auto& i : qreq->domains()) {
        auto d = m_domains->get(i);
        if(d) {
            domains.insert(d);
        }
        ds.insert(i);
    }
    auto qrsp = std::make_shared<QueryResponse>();
    for(auto& i : domains) {
        std::vector<NSNodeSet::ptr> nss;
        i->listAll(nss);
        for(auto& n : nss) {
            auto item = qrsp->add_infos();
            item->set_domain(i->getDomain());
            item->set_cmd(n->getCmd());
            std::vector<NSNode::ptr> ns;
            n->listAll(ns);

            for(auto& x : ns) {
                auto node = item->add_nodes();
                node->set_ip(x->getIp());
                node->set_port(x->getPort());
                node->set_weight(x->getWeight());
            }
        }
    }
    response->setResult(0);
    response->setResultStr("ok");
    response->setAsPB(*qrsp);
    //setQueryDomain(stream, ds);
    return true;
}

void NameServerModule::setQueryDomain(sylar::RockStream::ptr rs, const std::set<std::string>& ds) {
    std::set<std::string> old_ds;
    {
        sylar::RWMutex::ReadLock lock(m_mutex);
        auto it = m_queryDomains.find(rs);
        if(it != m_queryDomains.end()) {
            if(it->second == ds) {
                return;
            }
            old_ds = it->second;
        }
    }
    sylar::RWMutex::WriteLock lock(m_mutex);
    if(!rs->isConnected()) {
        return;
    }
    for(auto& i : old_ds) {
        m_domainToSessions[i].erase(rs);
    }
    for(auto& i : ds) {
        m_domainToSessions[i].insert(rs);
    }
    if(ds.empty()) {
        m_queryDomains.erase(rs);
    } else {
        m_queryDomains[rs] = ds;
    }
}

bool NameServerModule::handleTick(sylar::RockRequest::ptr request
                                  ,sylar::RockResponse::ptr response
                                  ,sylar::RockStream::ptr stream) {
    return true;
}

std::string NameServerModule::statusString() {
    std::stringstream ss;
    ss << RockModule::statusString() << std::endl;
    ss << "s_request_count: " << s_request_count << std::endl;
    ss << "s_on_connect: " << s_on_connect << std::endl;
    ss << "s_on_disconnect: " << s_on_disconnect << std::endl;
    m_domains->dump(ss);

    ss << "domainToSession: " << std::endl;
    sylar::RWMutex::ReadLock lock(m_mutex);
    for(auto& i : m_domainToSessions) {
        ss << "    " << i.first << ":" << std::endl;
        for(auto& v : i.second) {
            ss << "        " << v->getRemoteAddressString() << std::endl;
        }
        ss << std::endl;
    }
    return ss.str();
}

}
}
