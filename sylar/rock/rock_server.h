#ifndef __SYLAR_ROCK_SERVER_H__
#define __SYLAR_ROCK_SERVER_H__

#include "sylar/rock/rock_stream.h"
#include "sylar/tcp_server.h"

namespace sylar {

class RockServer : public TcpServer {
public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string& type = "rock"
               ,sylar::IOManager* worker = sylar::IOManager::GetThis()
               ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
               ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};

}

#endif
