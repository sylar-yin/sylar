#include "prometheus.h"
#include <prometheus/text_serializer.h>
#include <sstream>
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/http/http_connection.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

PrometheusRegistry::PrometheusRegistry() {
    m_register = std::make_shared<prometheus::Registry>();
}

prometheus::Family<prometheus::Counter>* PrometheusRegistry::addCounter(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_counters.find(name);
    if(it != m_counters.end()) {
        return it->second;
    }
    lock.unlock();
    auto rt = &prometheus::BuildCounter().Name(name).Help(help).Labels(labels).Register(*m_register);
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_counters[name] = rt;
        m_counterLabels[name];
    }
    return rt;
}

prometheus::Family<prometheus::Gauge>* PrometheusRegistry::addGauge(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_gauges.find(name);
    if(it != m_gauges.end()) {
        return it->second;
    }
    lock.unlock();
    auto rt = &prometheus::BuildGauge().Name(name).Help(help).Labels(labels).Register(*m_register);
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_gauges[name] = rt;
        m_gaugeLabels[name];
    }
    return rt;
}

prometheus::Family<prometheus::Histogram>* PrometheusRegistry::addHistogram(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_histograms.find(name);
    if(it != m_histograms.end()) {
        return it->second;
    }
    lock.unlock();
    auto rt = &prometheus::BuildHistogram().Name(name).Help(help).Labels(labels).Register(*m_register);
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_histograms[name] = rt;
        m_histogramLabels[name];
    }
    return rt;
}

prometheus::Family<prometheus::Summary>* PrometheusRegistry::addSummary(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_summarys.find(name);
    if(it != m_summarys.end()) {
        return it->second;
    }
    lock.unlock();
    auto rt = &prometheus::BuildSummary().Name(name).Help(help).Labels(labels).Register(*m_register);
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_summarys[name] = rt;
        m_summaryLabels[name];
    }
    return rt;
}

prometheus::Counter* PrometheusRegistry::addCounterLabels(const std::string& name, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    auto it = m_counterLabels.find(name);
    if(it == m_counterLabels.end()) {
        return nullptr;
    }
    auto iit = it->second.find(labels);
    if(iit != it->second.end()) {
        return iit->second;
    }
    auto xit = m_counters.find(name);
    if(xit == m_counters.end()) {
        return nullptr;
    }
    auto rt = &xit->second->Add(labels);
    lock.unlock();
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_counterLabels[name][labels] = rt;
    }
    return rt;
}

prometheus::Gauge* PrometheusRegistry::addGaugeLabels(const std::string& name, const std::map<std::string, std::string>& labels) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_gaugeLabels.find(name);
    if(it == m_gaugeLabels.end()) {
        return nullptr;
    }
    auto iit = it->second.find(labels);
    if(iit != it->second.end()) {
        return iit->second;
    }
    auto xit = m_gauges.find(name);
    if(xit == m_gauges.end()) {
        return nullptr;
    }
    auto rt = &xit->second->Add(labels);
    lock.unlock();
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_gaugeLabels[name][labels] = rt;
    }
    return rt;
}

prometheus::Histogram* PrometheusRegistry::addHistogramLabels(const std::string& name, const std::map<std::string, std::string>& labels, const std::vector<double>& buckets) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_histogramLabels.find(name);
    if(it == m_histogramLabels.end()) {
        return nullptr;
    }
    auto iit = it->second.find(labels);
    if(iit != it->second.end()) {
        return iit->second;
    }
    auto xit = m_histograms.find(name);
    if(xit == m_histograms.end()) {
        return nullptr;
    }
    auto rt = &xit->second->Add(labels, buckets);
    lock.unlock();
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_histogramLabels[name][labels] = rt;
    }
    return rt;
}

prometheus::Summary* PrometheusRegistry::addSummaryLabels(const std::string& name, const std::map<std::string, std::string>& labels, const prometheus::Summary::Quantiles& quantiles) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_summaryLabels.find(name);
    if(it == m_summaryLabels.end()) {
        return nullptr;
    }
    auto iit = it->second.find(labels);
    if(iit != it->second.end()) {
        return iit->second;
    }
    auto xit = m_summarys.find(name);
    if(xit == m_summarys.end()) {
        return nullptr;
    }
    auto rt = &xit->second->Add(labels, quantiles);
    lock.unlock();
    {
        sylar::RWMutex::WriteLock lock(m_mutex);
        m_summaryLabels[name][labels] = rt;
    }
    return rt;}

std::string PrometheusRegistry::toString() const {
    prometheus::TextSerializer ts;
    return ts.Serialize(m_register->Collect());
}

bool PrometheusClientConfig::operator==(const PrometheusClientConfig& o) const {
    return host == o.host
        && port == o.port
        && interval == o.interval
        && job == o.job
        && username == o.username
        && password == o.password
        && labels == o.labels;
}

PrometheusClient::PrometheusClient(const PrometheusClientConfig& config) {
    setConfig(config);
}

void PrometheusClient::addRegistry(const std::string& name, PrometheusRegistry::ptr data
                                   ,const std::map<std::string, std::string>& labels) {
    std::stringstream lb;
    for(auto& i : labels) {
        lb << "/" << i.first << "/" << i.second;
    }
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_infos[name] = std::make_pair(data, lb.str());
}

PrometheusRegistry::ptr PrometheusClient::getRegistry(const std::string& name) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_infos.find(name);
    return it == m_infos.end() ? nullptr : it->second.first;
}

void PrometheusClient::delRegistry(const std::string& name) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_infos.erase(name);
}

void PrometheusClient::setConfig(const PrometheusClientConfig& v) {
    m_config = v;
    std::stringstream ss;
    for(auto& i : v.labels) {
        ss << "/" << i.first << "/" << i.second;
    }
    m_labels = ss.str();
    {
        std::stringstream ss;
        ss << "http://" << m_config.host << ":" << m_config.port << "/metrics/job/" << m_config.job
           << m_labels;
        m_jobUri = ss.str();
    }
}

std::string PrometheusClient::getUri(const std::string& label) const {
    return m_jobUri + label;
}

void PrometheusClient::push() {
    prometheus::TextSerializer ts;
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto infos = m_infos;
    lock.unlock();

    for(auto& i : infos) {
        auto uri = getUri(i.second.second);
        auto str = i.second.first->toString();

        std::cout << "PrometheusClient name:" << i.first << std::endl;
        std::cout << "PrometheusClient uri:" << uri << std::endl;
        std::cout << "PrometheusClient JobUri:" << m_jobUri << std::endl;
        std::cout << "PrometheusClient label:" << i.second.second << std::endl;
        std::cout << "PrometheusClient body:" << str << std::endl;

        auto result = sylar::http::HttpConnection::DoPost(uri, 1000, {}, str);
        SYLAR_LOG_INFO(g_logger) << result->toString();
    }
}

void PrometheusClient::start() {
    auto self = shared_from_this();
    sylar::IOManager::GetThis()->schedule([self, this](){
        while(true) {
            push();
            sleep(m_config.interval);
        }
    });
}

}
