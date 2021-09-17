#ifndef __SYLAR_HTTP_SERVLETS_CONFIG_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_CONFIG_SERVLET_H__

#include "sylar/http/servlet.h"

namespace sylar {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;
};

}
}

#endif
