#ifndef __SYLAR_GRPC_GRPC_SERVER_H__
#define __SYLAR_GRPC_GRPC_SERVER_H__

#include "sylar/http2/http2_server.h"
#include "grpc_servlet.h"

namespace sylar {
namespace grpc {

class GrpcServer : public http2::Http2Server {
public:
    typedef std::shared_ptr<GrpcServer> ptr;
    GrpcServer(sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

    bool isStreamPath(const std::string& path);
    bool needSendResponse(const std::string& path);
    uint32_t getStreamPathType(const std::string& path);

    void addGrpcServlet(const std::string& path, GrpcServlet::ptr slt);
protected:
    void addStreamPath(const std::string& path, uint32_t type);
    virtual void handleClient(Socket::ptr client) override;
private:
    sylar::RWMutex m_mutex;
    std::unordered_map<std::string, uint32_t> m_streamTypes;
};

}
}

#endif
