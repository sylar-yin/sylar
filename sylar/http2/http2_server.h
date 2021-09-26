#ifndef __SYLAR_HTTP2_SERVER_H__
#define __SYLAR_HTTP2_SERVER_H__

#include "http2_stream.h"
#include "sylar/tcp_server.h"
#include "sylar/http/servlet.h"

namespace sylar {
namespace http2 {

class Http2Server : public TcpServer {
public:
    typedef std::shared_ptr<Http2Server> ptr;
    Http2Server(sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

    http::ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(http::ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;

    bool isStreamPath(const std::string& path);
    bool needSendResponse(const std::string& path);
    uint32_t getStreamPathType(const std::string& path);

    void addStreamPath(const std::string& path, uint32_t type);

    void addGrpcServlet(const std::string& path, sylar::http::Servlet::ptr slt);
    void addGrpcStreamClientServlet(const std::string& path, sylar::http::Servlet::ptr slt);
    void addGrpcStreamServerServlet(const std::string& path, sylar::http::Servlet::ptr slt);
    void addGrpcStreamBothServlet(const std::string& path, sylar::http::Servlet::ptr slt);
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    http::ServletDispatch::ptr m_dispatch;
    sylar::RWMutex m_mutex;
    std::unordered_map<std::string, uint32_t> m_streamTypes;
};

}
}

#endif
