#ifndef __SYLAR_KAFKA_CLIENT_H__
#define __SYLAR_KAFKA_CLIENT_H__

#include <memory>
#include <librdkafka/rdkafka.h>
#include <librdkafka/rdkafkacpp.h>
#include <map>
#include <string>
#include "sylar/mutex.h"
#include "sylar/iomanager.h"

namespace sylar {

class MyEventCb;
class MyDeliveryReportCb;

class KafkaTopic {
public:
    typedef std::shared_ptr<RdKafka::Topic> topic_ptr;
    typedef std::shared_ptr<KafkaTopic> ptr;

    KafkaTopic(topic_ptr t);

    topic_ptr getTopic() const { return m_topic;}
    uint64_t getCount() const { return m_count;}

    uint64_t incCount();
private:
    topic_ptr m_topic;
    uint64_t m_count;
};

class KafkaProducer {
public:
    typedef std::shared_ptr<KafkaProducer> ptr;
    typedef std::shared_ptr<RdKafka::Conf> conf_ptr;
    typedef std::shared_ptr<RdKafka::Producer> producer_ptr;

    KafkaProducer();
    ~KafkaProducer();

    bool setBrokerList(const std::string& bl);
    bool produce(const std::string& topic, const std::string& msg);
    bool produce(const std::string& topic, const std::string& msg, const std::string& key);
    std::string getErr() const { return m_err;}

    std::string getBrokerList() const { return m_brokerList;}
    void listTopics(std::map<std::string, uint64_t>& topics);

    bool isConnected() const { return m_connected;}

    uint64_t getTotalMsg() const { return m_totalMsg;}
    uint64_t getSuccessMsg() const { return m_sucessMsg;}

    void poll(uint64_t t = 0);
private:
    KafkaTopic::ptr getTopic(const std::string& topic);
private:
    sylar::RWMutex m_mutex;
    std::string m_brokerList;
    conf_ptr m_conf;
    conf_ptr m_tconf;
    producer_ptr m_producer;
    std::map<std::string, KafkaTopic::ptr> m_topics;
    std::string m_err;

    bool m_connected;
    MyEventCb* m_eventCb;
    MyDeliveryReportCb* m_drCb;
    
    uint64_t m_totalMsg;
    uint64_t m_sucessMsg;
};

class KafkaConsumer{
public:
    typedef std::shared_ptr<KafkaConsumer> ptr;
    typedef std::shared_ptr<RdKafka::Conf> conf_ptr;
    typedef std::shared_ptr<RdKafka::Consumer> consumer_ptr;
    typedef std::function<void(const RdKafka::Message& msg, KafkaConsumer* consumer)> message_cb;

    KafkaConsumer();
    ~KafkaConsumer();

    bool setBrokerList(const std::string& bl);
    //bool produce(const std::string& topic, const std::string& msg);
    //bool produce(const std::string& topic, const std::string& msg, const std::string& key);
    std::string getErr() const { return m_err;}

    std::string getBrokerList() const { return m_brokerList;}
    KafkaTopic::ptr getTopic() const { return m_topic;}

    bool isConnected() const { return m_connected;}

    uint64_t getTotalMsg() const { return m_totalMsg;}
    uint64_t getSuccessMsg() const { return m_sucessMsg;}

    void setCallback(message_cb cb) { m_cb = cb;}
    message_cb getCallback() const { return m_cb;}

    void poll(uint64_t t = 0);
    void start(const std::string& topic_name,
               int32_t partition = RdKafka::Topic::PARTITION_UA,
               int64_t start_offset = RdKafka::Topic::OFFSET_BEGINNING,
               const std::string& offset_path = "");

    void updateOffset(const RdKafka::Message& msg);

    void stop();

    bool isCcb() const { return m_ccb;}
    void setCcb(bool v) { m_ccb = v;}

    void dump(std::ostream& os);

    void listOffset(std::map<uint32_t, int64_t>& out);
private:
    sylar::RWMutex m_mutex;
    std::string m_brokerList;
    conf_ptr m_conf;
    consumer_ptr m_consumer;
    KafkaTopic::ptr m_topic;
    std::string m_err;

    bool m_connected;
    bool m_running;
    bool m_ccb;
    MyEventCb* m_eventCb;
    
    uint64_t m_totalMsg;
    uint64_t m_sucessMsg;

    int32_t m_partition;
    int64_t m_startOffset;
    int64_t m_curOffset;

    std::map<uint32_t, int64_t> m_partitionOffsets;
    std::map<uint32_t, int64_t> m_partitionTotal;
    std::map<uint32_t, int64_t> m_partitionStartOffset;

    message_cb m_cb;
};

class KafkaConsumerGroup {
public:
    typedef std::shared_ptr<KafkaConsumerGroup> ptr;
    typedef std::function<void(const RdKafka::Message& msg, KafkaConsumer* consumer)> message_cb;

    KafkaConsumerGroup();
    ~KafkaConsumerGroup();

    void setCallback(message_cb cb) { m_cb = cb;}
    void setBrokerList(const std::string& bl) { m_brokerList = bl;}

    const std::string getBrokerList() const { return m_brokerList;}

    const std::string& getThreadName() const { return m_threadName;}
    const std::string& getTopic() const { return m_topic;}
    uint32_t getPartitionStart() const { return m_partitionStart;}
    uint32_t getPartitionEnd() const { return m_partitionEnd;}
    uint32_t getPartitionShare() const { return m_partitionShare;}

    void setThreadName(const std::string& val) { m_threadName = val;};
    void setPartitionStart(const uint32_t& val) { m_partitionStart = val;}
    void setPartitionEnd(const uint32_t& val) { m_partitionEnd = val;}
    void setPartitionShare(const uint32_t& val) { m_partitionShare = val;}
    void setTopic(const std::string& val) { m_topic = val;}

    void setOffsets(const std::map<uint32_t, int64_t>& val) { m_offsets = val;}
    void listOffsets(std::map<uint32_t, int64_t>& vs);
    void start();

    void setDefaultOffset(int32_t val) { m_defaultOffset = val;}

    std::ostream& dump(std::ostream& os);

    uint64_t getTotalMsg();

    int32_t getSnapInterval() const { return m_snapInterval;}
    const std::string& getSnapFilePath() const { return m_snapFilePath;}

    //seconds
    void setSnapInterval(int32_t val) { m_snapInterval = val;}
    void setSnapFilePath(const std::string& val) { m_snapFilePath = val;}
private:
    std::string m_threadName;
    std::string m_topic;
    uint32_t m_partitionStart;
    uint32_t m_partitionEnd;
    uint32_t m_partitionShare;
    int32_t m_defaultOffset;
    int32_t m_snapInterval;
    std::string m_snapFilePath;
    
    sylar::Timer::ptr m_timer;
    std::map<uint32_t, int64_t> m_offsets;
    message_cb m_cb;
    std::string m_brokerList;
    sylar::RWMutex m_mutex;
    std::vector<KafkaConsumer::ptr> m_datas;
};

}

#endif
