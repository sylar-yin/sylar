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
    Http2Server(const std::string& type = "http2"
                ,sylar::IOManager* worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* io_worker = sylar::IOManager::GetThis()
                ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

    http::ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(http::ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    http::ServletDispatch::ptr m_dispatch;
};

}
}

#endif
