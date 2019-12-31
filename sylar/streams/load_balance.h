#ifndef __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__
#define __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__

#include "sylar/streams/socket_stream.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include "sylar/streams/service_discovery.h"
#include <vector>
#include <unordered_map>

namespace sylar {

class HolderStatsSet;
class HolderStats {
friend class HolderStatsSet;
public:
    uint32_t getUsedTime() const { return m_usedTime; }
    uint32_t getTotal() const { return m_total; }
    uint32_t getDoing() const { return m_doing; }
    uint32_t getTimeouts() const { return m_timeouts; }
    uint32_t getOks() const { return m_oks; }
    uint32_t getErrs() const { return m_errs; }

    uint32_t incUsedTime(uint32_t v) { return sylar::Atomic::addFetch(m_usedTime ,v);}
    uint32_t incTotal(uint32_t v) { return sylar::Atomic::addFetch(m_total, v);}
    uint32_t incDoing(uint32_t v) { return sylar::Atomic::addFetch(m_doing, v);}
    uint32_t incTimeouts(uint32_t v) { return sylar::Atomic::addFetch(m_timeouts, v);}
    uint32_t incOks(uint32_t v) { return sylar::Atomic::addFetch(m_oks, v);}
    uint32_t incErrs(uint32_t v) { return sylar::Atomic::addFetch(m_errs, v);}

    uint32_t decDoing(uint32_t v) { return sylar::Atomic::subFetch(m_doing, v);}
    void clear();

    float getWeight(float rate = 1.0f);

    std::string toString();
private:
    uint32_t m_usedTime = 0;
    uint32_t m_total = 0;
    uint32_t m_doing = 0;
    uint32_t m_timeouts = 0;
    uint32_t m_oks = 0;
    uint32_t m_errs = 0;
};

class HolderStatsSet {
public:
    HolderStatsSet(uint32_t size = 5);
    HolderStats& get(const uint32_t& now = time(0));

    float getWeight(const uint32_t& now = time(0));

    HolderStats getTotal();
private:
    void init(const uint32_t& now);
private:
    uint32_t m_lastUpdateTime = 0; //seconds
    std::vector<HolderStats> m_stats;
};

class LoadBalanceItem {
public:
    typedef std::shared_ptr<LoadBalanceItem> ptr;
    virtual ~LoadBalanceItem() {}

    SocketStream::ptr getStream() const { return m_stream;}
    void setStream(SocketStream::ptr v) { m_stream = v;}

    void setId(uint64_t v) { m_id = v;}
    uint64_t getId() const { return m_id;}

    HolderStats& get(const uint32_t& now = time(0));

    template<class T>
    std::shared_ptr<T> getStreamAs() {
        return std::dynamic_pointer_cast<T>(m_stream);
    }

    virtual int32_t getWeight() { return m_weight;}
    void setWeight(int32_t v) { m_weight = v;}

    virtual bool isValid();
    void close();

    std::string toString();
protected:
    uint64_t m_id = 0;
    SocketStream::ptr m_stream;
    int32_t m_weight = 0;
    HolderStatsSet m_stats;
};

class ILoadBalance {
public:
    enum Type {
        ROUNDROBIN = 1,
        WEIGHT = 2,
        FAIR = 3
    };

    enum Error {
        NO_SERVICE = -101,
        NO_CONNECTION = -102,
    };
    typedef std::shared_ptr<ILoadBalance> ptr;
    virtual ~ILoadBalance() {}
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) = 0;
};

class LoadBalance : public ILoadBalance {
public:
    typedef sylar::RWSpinlock RWMutexType;
    typedef std::shared_ptr<LoadBalance> ptr;
    void add(LoadBalanceItem::ptr v);
    void del(LoadBalanceItem::ptr v);
    void set(const std::vector<LoadBalanceItem::ptr>& vs);

    LoadBalanceItem::ptr getById(uint64_t id);
    void update(const std::unordered_map<uint64_t, LoadBalanceItem::ptr>& adds
                ,std::unordered_map<uint64_t, LoadBalanceItem::ptr>& dels);
    void init();

    std::string statusString(const std::string& prefix);
protected:
    virtual void initNolock() = 0;
    void checkInit();
protected:
    RWMutexType m_mutex;
    std::unordered_map<uint64_t, LoadBalanceItem::ptr> m_datas;
    uint64_t m_lastInitTime = 0;
};

class RoundRobinLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<RoundRobinLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;
protected:
    virtual void initNolock();
protected:
    std::vector<LoadBalanceItem::ptr> m_items;
};

//class FairLoadBalance;
class FairLoadBalanceItem : public LoadBalanceItem {
//friend class FairLoadBalance;
public:
    typedef std::shared_ptr<FairLoadBalanceItem> ptr;

    void clear();
    virtual int32_t getWeight();
};

class WeightLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<WeightLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get(uint64_t v = -1) override;

    FairLoadBalanceItem::ptr getAsFair();
protected:
    virtual void initNolock();
private:
    int32_t getIdx(uint64_t v = -1);
protected:
    std::vector<LoadBalanceItem::ptr> m_items;
private:
    std::vector<int64_t> m_weights;
};



//class FairLoadBalance : public LoadBalance {
//public:
//    typedef std::shared_ptr<FairLoadBalance> ptr;
//    virtual LoadBalanceItem::ptr get() override;
//    FairLoadBalanceItem::ptr getAsFair();
//
//protected:
//    virtual void initNolock();
//private:
//    int32_t getIdx();
//protected:
//    std::vector<LoadBalanceItem::ptr> m_items;
//private:
//    std::vector<int32_t> m_weights;
//};

class SDLoadBalance {
public:
    typedef std::shared_ptr<SDLoadBalance> ptr;
    typedef std::function<SocketStream::ptr(ServiceItemInfo::ptr)> stream_callback;
    typedef sylar::RWSpinlock RWMutexType;

    SDLoadBalance(IServiceDiscovery::ptr sd);
    virtual ~SDLoadBalance() {}

    virtual void start();
    virtual void stop();

    stream_callback getCb() const { return m_cb;}
    void setCb(stream_callback v) { m_cb = v;}

    LoadBalance::ptr get(const std::string& domain, const std::string& service, bool auto_create = false);

    void initConf(const std::unordered_map<std::string, std::unordered_map<std::string, std::string> >& confs);

    std::string statusString();
private:
    void onServiceChange(const std::string& domain, const std::string& service
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& old_value
                ,const std::unordered_map<uint64_t, ServiceItemInfo::ptr>& new_value);

    ILoadBalance::Type getType(const std::string& domain, const std::string& service);
    LoadBalance::ptr createLoadBalance(ILoadBalance::Type type);
    LoadBalanceItem::ptr createLoadBalanceItem(ILoadBalance::Type type);
protected:
    RWMutexType m_mutex;
    IServiceDiscovery::ptr m_sd;
    //domain -> [ service -> [ LoadBalance ] ]
    std::unordered_map<std::string, std::unordered_map<std::string, LoadBalance::ptr> > m_datas;
    std::unordered_map<std::string, std::unordered_map<std::string, ILoadBalance::Type> > m_types;
    ILoadBalance::Type m_defaultType = ILoadBalance::FAIR;
    stream_callback m_cb;
};

}

#endif
