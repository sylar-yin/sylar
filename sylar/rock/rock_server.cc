#include "rock_server.h"
#include "sylar/log.h"
#include "sylar/module.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

RockServer::RockServer(sylar::IOManager* worker
                       ,sylar::IOManager* accept_worker)
    :TcpServer(worker, accept_worker) {
    m_type = "rock";
}

void RockServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
    sylar::RockSession::ptr session(new sylar::RockSession(client));
    ModuleMgr::GetInstance()->foreach(Module::ROCK,
            [session](Module::ptr m) {
        m->onConnect(session);
    });
    session->setDisconnectCb(
        [](AsyncSocketStream::ptr stream) {
             ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [stream](Module::ptr m) {
                m->onDisconnect(stream);
            });
        }
    );
    session->setRequestHandler(
        [](sylar::RockRequest::ptr req
           ,sylar::RockResponse::ptr rsp
           ,sylar::RockStream::ptr conn)->bool {
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, req, rsp, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                auto rm = std::dynamic_pointer_cast<RockModule>(m);
                if(rm) {
                    rt = rm->handle(req, rsp, conn);
                }
            });
            return rt;
        }
    ); 
    session->setNotifyHandler(
        [](sylar::RockNotify::ptr nty
           ,sylar::RockStream::ptr conn)->bool {
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, nty, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                auto rm = std::dynamic_pointer_cast<RockModule>(m);
                if(rm) {
                    rt = rm->handle(nty, conn);
                }
            });
            return rt;
        }
    );
    session->start();
}

}
