#include "status_servlet.h"
#include "sylar/sylar.h"

namespace sylar {
namespace http {

static const char* GetFiberTypeStr() {
    if(FIBER_CONTEXT_TYPE == FIBER_UCONTEXT) {
        return "FIBER_UCONTEXT";
    } else if(FIBER_CONTEXT_TYPE == FIBER_FCONTEXT) {
        return "FIBER_FCONTEXT";
    } else if(FIBER_CONTEXT_TYPE == FIBER_LIBCO) {
        return "FIBER_LIBCO";
    } else if(FIBER_CONTEXT_TYPE == FIBER_LIBACO) {
        return "FIBER_LIBACO";
    }
    return "UNKNOW";
}

StatusServlet::StatusServlet()
    :Servlet("StatusServlet") {
}

std::string format_used_time(int64_t ts) {
    std::stringstream ss;
    bool v = false;
    if(ts >= 3600 * 24) {
        ss << (ts / 3600 / 24) << "d ";
        ts = ts % (3600 * 24);
        v = true;
    }
    if(ts >= 3600) {
        ss << (ts / 3600) << "h ";
        ts = ts % 3600;
        v = true;
    } else if(v) {
        ss << "0h ";
    }

    if(ts >= 60) {
        ss << (ts / 60) << "m ";
        ts = ts % 60;
    } else if(v) {
        ss << "0m ";
    }
    ss << ts << "s";
    return ss.str();
}

int32_t StatusServlet::handle(sylar::http::HttpRequest::ptr request
                              ,sylar::http::HttpResponse::ptr response
                              ,sylar::http::HttpSession::ptr session) {
    response->setHeader("Content-Type", "text/text; charset=utf-8");
#define XX(key) \
    ss << std::setw(30) << std::right << key ": "
    std::stringstream ss;
    ss << "===================================================" << std::endl;
    XX("server_version") << "sylar/1.0.0" << std::endl;
    
    std::vector<Module::ptr> ms;
    ModuleMgr::GetInstance()->listAll(ms);

    XX("modules");
    for(size_t i = 0; i < ms.size(); ++i) {
        if(i) {
            ss << ";";
        }
        ss << ms[i]->getId();
    }
    ss << std::endl;
    XX("host") << GetHostName() << std::endl;
    XX("ipv4") << GetIPv4() << std::endl;
    XX("daemon_id") << ProcessInfoMgr::GetInstance()->parent_id << std::endl;
    XX("main_id") << ProcessInfoMgr::GetInstance()->main_id << std::endl;
    XX("daemon_start") << Time2Str(ProcessInfoMgr::GetInstance()->parent_start_time) << std::endl;
    XX("main_start") << Time2Str(ProcessInfoMgr::GetInstance()->main_start_time) << std::endl;
    XX("restart_count") << ProcessInfoMgr::GetInstance()->restart_count << std::endl;
    XX("daemon_running_time") << format_used_time(time(0) - ProcessInfoMgr::GetInstance()->parent_start_time) << std::endl;
    XX("main_running_time") << format_used_time(time(0) - ProcessInfoMgr::GetInstance()->main_start_time) << std::endl;
    ss << "===================================================" << std::endl;
    XX("fiber_type") << GetFiberTypeStr() << std::endl;
    XX("fibers") << sylar::Fiber::TotalFibers() << std::endl;
    ss << "===================================================" << std::endl;
    ss << "<Logger>" << std::endl;
    ss << sylar::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    ss << "===================================================" << std::endl;
    ss << "<Woker>" << std::endl;
    sylar::WorkerMgr::GetInstance()->dump(ss) << std::endl;

    std::map<std::string, std::vector<TcpServer::ptr> > servers;
    sylar::Application::GetInstance()->listAllServer(servers);
    ss << "===================================================" << std::endl;
    for(auto it = servers.begin();
            it != servers.end(); ++it) {
        if(it != servers.begin()) {
            ss << "***************************************************" << std::endl;
        }
        ss << "<Server." << it->first << ">" << std::endl;
        sylar::http::HttpServer::ptr hs;
        for(auto iit = it->second.begin();
                iit != it->second.end(); ++iit) {
            if(iit != it->second.begin()) {
                ss << "---------------------------------------------------" << std::endl;
            }
            if(!hs) {
                hs = std::dynamic_pointer_cast<sylar::http::HttpServer>(*iit);
            }
            ss << (*iit)->toString() << std::endl;
        }
        if(hs) {
            auto sd = hs->getServletDispatch();
            if(sd) {
                std::map<std::string, IServletCreator::ptr> infos;
                sd->listAllServletCreator(infos);
                if(!infos.empty()) {
                    ss << "[Servlets]" << std::endl;
#define XX2(key) \
    ss << std::setw(30) << std::right << key << ": "
                    for(auto& i : infos) {
                        XX2(i.first) << i.second->getName() << std::endl;
                    }
                    infos.clear();
                }
                sd->listAllGlobServletCreator(infos);
                if(!infos.empty()) {
                    ss << "[Servlets.Globs]" << std::endl;
                    for(auto& i : infos) {
                        XX2(i.first) << i.second->getName() << std::endl;
                    }
                    infos.clear();
                }
            }
        }
    }
    ss << "===================================================" << std::endl;
    ss << "<ServiceDiscovery>" << std::endl;
    auto sd = Application::GetInstance()->getServiceDiscovery();
    if(sd) {
        ss << sd->toString() << std::endl;
    }
    ss << "===================================================" << std::endl;
    ss << "<LoadBalance>" << std::endl;
    auto rlb = Application::GetInstance()->getRockSDLoadBalance();
    if(rlb) {
        ss << rlb->statusString() << std::endl;
    }
    auto glb = Application::GetInstance()->getGrpcSDLoadBalance();
    if(glb) {
        ss << glb->statusString() << std::endl;
    }

    ss << "===================================================" << std::endl;
    for(size_t i = 0; i < ms.size(); ++i) {
        if(i) {
            ss << "***************************************************" << std::endl;
        }
        ss << ms[i]->statusString() << std::endl;
    }

    response->setBody(ss.str());
    return 0;
}

}
}
