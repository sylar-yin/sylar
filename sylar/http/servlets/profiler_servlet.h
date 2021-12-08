#ifndef __SYLAR_HTTP_SERVLET_PROFILER_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_PROFILER_SERVLET_H__

#include "sylar/http/servlet.h"

namespace sylar {
namespace http {

class ProfilerServlet : public Servlet {
public:
    ProfilerServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;
};


}
}

#endif
