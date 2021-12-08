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
    prometheus::TextSerializer ts;
    std::stringstream ss;
    ts.Serialize(ss, registry->Collect());
    response->setBody(ss.str());
    return 0;
}

std::shared_ptr<prometheus::Registry> GetPrometheusRegistry() {
    static std::shared_ptr<prometheus::Registry> s_instance = std::make_shared<prometheus::Registry>();
    return s_instance;
}

}
}
