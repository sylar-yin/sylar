#ifndef __SYLAR_HTTP_SERVLETS_METRICS_SERVLET_H__
#define __SYLAR_HTTP_SERVLETS_METRICS_SERVLET_H__

#include "sylar/http/servlet.h"
#include <prometheus/registry.h>
#include <prometheus/counter.h>

namespace sylar {
namespace http {

class MetricsServlet : public Servlet {
public:
    MetricsServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;
};

std::shared_ptr<prometheus::Registry> GetPrometheusRegistry();

}
}

#endif
