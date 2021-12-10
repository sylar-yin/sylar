#include "metrics_servlet.h"
#include <prometheus/text_serializer.h>

namespace sylar {
namespace http {

MetricsServlet::MetricsServlet()
    :Servlet("MetricsServlet") {
}

int32_t MetricsServlet::handle(sylar::http::HttpRequest::ptr request
                               , sylar::http::HttpResponse::ptr response
                               , sylar::SocketStream::ptr session) {
    auto registry = GetPrometheusRegistry();
    //auto& counter = prometheus::BuildCounter().Name("sylar_test_counter")
    //                    .Help("Number of counter").Register(*registry);
    //counter.Add({{"url", request->getPath()}}).Increment();
    response->setBody(registry->toString());
    return 0;
}

PrometheusRegistry::ptr GetPrometheusRegistry() {
    static PrometheusRegistry::ptr s_instance = std::make_shared<PrometheusRegistry>();
    return s_instance;
}

}
}
