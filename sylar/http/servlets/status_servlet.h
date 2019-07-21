#ifndef __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_STATUS_SERVLET_H__

#include "sylar/http/servlet.h"

namespace sylar {
namespace http {

class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;
};

}
}

#endif
