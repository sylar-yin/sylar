#include "sylar/sylar.h"
#include "sylar/util/prometheus.h"
#include <prometheus/counter.h>
#include <prometheus/registry.h>
#include <prometheus/text_serializer.h>

SYLAR_DEFINE_CONFIG(sylar::PrometheusClientConfig, s_cfg, "prometheus_client", {}, "prometheus cfg");

void run() {
    sylar::Config::LoadFromConfDir("conf");
    auto registry = std::make_shared<sylar::PrometheusRegistry>();
    sylar::PrometheusClient::ptr client = std::make_shared<sylar::PrometheusClient>(s_cfg->getValue());

    //auto& counter = prometheus::BuildCounter().Name("sylar_test_counter")
    //                    .Labels({{"key","val"}})
    //                    .Help("Number of counter").Register(*registry);
    //counter.Add({{"url", "haha"},{"code","200"}}).Increment();
    registry->addCounter("sylar_test_counter", "Number of counter", {{"key", "val"}});
    //counter.Add({{"url", "haha"},{"code","200"}}).Increment();
    registry->addCounterLabels("sylar_test_counter", {{"url", "/test"}})->Increment();

    //prometheus::TextSerializer ts;
    //auto str = ts.Serialize(registry->Collect());
    //std::cout << str << std::endl;
    std::cout << registry->toString() << std::endl;

    std::cout << s_cfg->toString() << std::endl;
    client->addRegistry("test", registry, {{"key", "val"}});
    client->start();

    std::map<std::map<std::string, std::string>, std::string> mm;
}

int main(int argc, char** argv) {
    sylar::EnvMgr::GetInstance()->init(argc, argv);
    sylar::IOManager iom(2);
    iom.schedule(run);
    iom.addTimer(1000, [](){}, true);
    iom.stop();
    return 0;
}
