#include "sylar/sylar.h"
#include "sylar/ns/ns_protocol.h"
#include "sylar/ns/ns_client.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int type = 0;

void run() {
    g_logger->setLevel(sylar::LogLevel::INFO);
    auto addr = sylar::IPAddress::Create("127.0.0.1", 8072);
    //if(!conn->connect(addr)) {
    //    SYLAR_LOG_ERROR(g_logger) << "connect to: " << *addr << " fail";
    //    return;
    //}
    if(type == 0) {
        for(int i = 0; i < 5000; ++i) {
            sylar::RockConnection::ptr conn(new sylar::RockConnection);
            conn->connect(addr);
            sylar::IOManager::GetThis()->addTimer(3000, [conn, i](){
                    sylar::RockRequest::ptr req(new sylar::RockRequest);
                    req->setCmd((int)sylar::ns::NSCommand::REGISTER);
                    auto rinfo = std::make_shared<sylar::ns::RegisterRequest>();
                    auto info = rinfo->add_infos();
                    info->set_domain(std::to_string(rand() % 2) + "domain.com");
                    info->add_cmds(rand() % 2 + 100);
                    info->add_cmds(rand() % 2 + 200);
                    info->mutable_node()->set_ip("127.0.0.1");
                    info->mutable_node()->set_port(1000 + i);
                    info->mutable_node()->set_weight(100);
                    req->setAsPB(*rinfo);

                    auto rt = conn->request(req, 100);
                    SYLAR_LOG_INFO(g_logger) << "[result="
                        << rt->result << " response="
                        << (rt->response ? rt->response->toString() : "null")
                        << "]";
            }, true);
            conn->start();
        }
    } else {
        for(int i = 0; i < 1000; ++i) {
            sylar::ns::NSClient::ptr nsclient(new sylar::ns::NSClient);
            nsclient->init();
            nsclient->addQueryDomain(std::to_string(i % 2) + "domain.com");
            nsclient->connect(addr);
            nsclient->start();
            SYLAR_LOG_INFO(g_logger) << "NSClient start: i=" << i;

            if(i == 0) {
                //sylar::IOManager::GetThis()->addTimer(1000, [nsclient](){
                //    auto domains = nsclient->getDomains();
                //    domains->dump(std::cout, "    ");
                //}, true);
            }
        }

        //conn->setConnectCb([](sylar::AsyncSocketStream::ptr ss) {
        //    auto conn = std::dynamic_pointer_cast<sylar::RockConnection>(ss);
        //    sylar::RockRequest::ptr req(new sylar::RockRequest);
        //    req->setCmd((int)sylar::ns::NSCommand::QUERY);
        //    auto rinfo = std::make_shared<sylar::ns::QueryRequest>();
        //    rinfo->add_domains("0domain.com");
        //    req->setAsPB(*rinfo);
        //    auto rt = conn->request(req, 1000);
        //    SYLAR_LOG_INFO(g_logger) << "[result="
        //        << rt->result << " response="
        //        << (rt->response ? rt->response->toString() : "null")
        //        << "]";
        //    return true;
        //});

        //conn->setNotifyHandler([](sylar::RockNotify::ptr nty,sylar::RockStream::ptr stream){
        //        auto nm = nty->getAsPB<sylar::ns::NotifyMessage>();
        //        if(!nm) {
        //            SYLAR_LOG_ERROR(g_logger) << "invalid notify message";
        //            return true;
        //        }
        //        SYLAR_LOG_INFO(g_logger) << sylar::PBToJsonString(*nm);
        //        return true;
        //});
    }
}

int main(int argc, char** argv) {
    if(argc > 1) {
        type = 1;
    }
    sylar::IOManager iom(5);
    iom.schedule(run);
    return 0;
}
