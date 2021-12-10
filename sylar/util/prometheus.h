#ifndef __SYLAR_UTIL_PROMETHEUS_H__
#define __SYLAR_UTIL_PROMETHEUS_H__

#include "sylar/pack/pack.h"
#include "sylar/mutex.h"
#include <map>
#include <memory>
#include <string>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>

namespace sylar {

struct PrometheusClientConfig {
    std::string host;
    uint16_t port = 0;
    uint16_t interval = 10;
    std::string job;
    std::string username;
    std::string password;

    std::map<std::string, std::string> labels;

    SYLAR_PACK(O(host, port, job, username, password, labels, interval));
    bool operator==(const PrometheusClientConfig& o) const;
};

class PrometheusRegistry {
public:
    typedef std::shared_ptr<PrometheusRegistry> ptr;
    PrometheusRegistry();

    prometheus::Family<prometheus::Counter>* addCounter(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels);
    prometheus::Family<prometheus::Gauge>* addGauge(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels);
    prometheus::Family<prometheus::Histogram>* addHistogram(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels);
    prometheus::Family<prometheus::Summary>* addSummary(const std::string& name, const std::string& help, const std::map<std::string, std::string>& labels);

    prometheus::Counter* addCounterLabels(const std::string& name, const std::map<std::string, std::string>& labels);
    prometheus::Gauge* addGaugeLabels(const std::string& name, const std::map<std::string, std::string>& labels);
    prometheus::Histogram* addHistogramLabels(const std::string& name, const std::map<std::string, std::string>& labels, const std::vector<double>& buckets);
    prometheus::Summary* addSummaryLabels(const std::string& name, const std::map<std::string, std::string>& labels, const prometheus::Summary::Quantiles& quantiles);

    std::string toString() const;
private:
    sylar::RWMutex m_mutex;
    std::shared_ptr<prometheus::Registry> m_register;
    std::map<std::string, prometheus::Family<prometheus::Counter>* > m_counters;
    std::map<std::string, prometheus::Family<prometheus::Gauge>* > m_gauges;
    std::map<std::string, prometheus::Family<prometheus::Histogram>* > m_histograms;
    std::map<std::string, prometheus::Family<prometheus::Summary>* > m_summarys;

    std::map<std::string, std::map<std::map<std::string, std::string>, prometheus::Counter* > > m_counterLabels;
    std::map<std::string, std::map<std::map<std::string, std::string>, prometheus::Gauge* > > m_gaugeLabels;
    std::map<std::string, std::map<std::map<std::string, std::string>, prometheus::Histogram* > > m_histogramLabels;
    std::map<std::string, std::map<std::map<std::string, std::string>, prometheus::Summary* > > m_summaryLabels;
};

class PrometheusClient : public std::enable_shared_from_this<PrometheusClient> {
public:
    typedef std::shared_ptr<PrometheusClient> ptr;
    typedef std::pair<PrometheusRegistry::ptr, std::string> RegistryInfo;
    PrometheusClient(const PrometheusClientConfig& config);

    const PrometheusClientConfig& getConfig() const { return m_config;}
    void setConfig(const PrometheusClientConfig& v);

    void addRegistry(const std::string& name, PrometheusRegistry::ptr data
                     ,const std::map<std::string, std::string>& labels = {});
    PrometheusRegistry::ptr getRegistry(const std::string& name);
    void delRegistry(const std::string& name);

    void push();
    void start();
private:
    std::string getUri(const std::string& label) const;
private:
    PrometheusClientConfig m_config;
    sylar::RWMutex m_mutex;
    std::map<std::string, RegistryInfo> m_infos;
    std::string m_labels;
    std::string m_jobUri;
};

}

#endif
