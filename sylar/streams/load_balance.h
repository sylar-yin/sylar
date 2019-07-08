#ifndef __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__
#define __SYLAR_STREAMS_SOCKET_STREAM_POOL_H__

#include "sylar/streams/socket_stream.h"
#include "sylar/mutex.h"
#include "sylar/util.h"
#include <vector>

namespace sylar {

class LoadBalanceItem {
public:
    typedef std::shared_ptr<LoadBalanceItem> ptr;
    virtual ~LoadBalanceItem() {}

    SocketStream::ptr getStream() const { return m_stream;}

    template<class T>
    std::shared_ptr<T> getStreamAs() {
        return std::dynamic_pointer_cast<T>(m_stream);
    }

    virtual int32_t getWeight() { return m_weight;}
    void setWeight(int32_t v) { m_weight = v;}

    virtual bool isValid() = 0;
protected:
    SocketStream::ptr m_stream;
    int32_t m_weight = 0;
};

class LoadBalance {
public:
    typedef sylar::RWMutex RWMutexType;
    typedef std::shared_ptr<LoadBalance> ptr;
    virtual ~LoadBalance() {}

    virtual LoadBalanceItem::ptr get() = 0;
    void add(LoadBalanceItem::ptr v);
    void del(LoadBalanceItem::ptr v);
    void set(const std::vector<LoadBalanceItem::ptr>& vs);
protected:
    RWMutexType m_mutex;
    std::vector<LoadBalanceItem::ptr> m_items;
};

class RoundRobinLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<RoundRobinLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get() override;
};

class WeightLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<WeightLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get() override;

    int32_t initWeight();
private:
    int32_t getIdx();
private:
    std::vector<int32_t> m_weights;
};

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
    uint32_t incErrs(uint32_t v) { return sylar::Atomic::addFetch(m_errs, 0);}

    uint32_t decDoing(uint32_t v) { return sylar::Atomic::subFetch(m_doing, 0);}
    void clear();

    float getWeight(float rate = 1.0f);
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
private:
    void init(const uint32_t& now);
private:
    uint32_t m_lastUpdateTime = 0; //seconds
    std::vector<HolderStats> m_stats;
};

class FairLoadBalance;
class FairLoadBalanceItem : public LoadBalanceItem {
friend class FairLoadBalance;
public:
    typedef std::shared_ptr<FairLoadBalanceItem> ptr;

    void clear();
    virtual int32_t getWeight();
    HolderStats& get(const uint32_t& now = time(0));
protected:
    HolderStatsSet m_stats;
};

class FairLoadBalance : public LoadBalance {
public:
    typedef std::shared_ptr<FairLoadBalance> ptr;
    virtual LoadBalanceItem::ptr get() override;
    FairLoadBalanceItem::ptr getAsFair();

    int32_t initWeight();
private:
    int32_t getIdx();
private:
    std::vector<int32_t> m_weights;
};

}

#endif
