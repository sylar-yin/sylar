#ifndef __SYLAR_DS_BLOCKING_QUEUE_H__
#define __SYLAR_DS_BLOCKING_QUEUE_H__

#include "sylar/mutex.h"

namespace sylar {
namespace ds {

template<class T>
class BlockingQueue {
public:
    typedef std::shared_ptr<BlockingQueue> ptr;
    typedef std::shared_ptr<T> data_type;

    size_t push(const data_type& data) {
        sylar::Mutex::Lock lock(m_mutex);
        m_datas.push_back(data);
        size_t size = m_datas.size();
        lock.unlock();
        m_sem.notify();
        return size;
    }

    data_type pop() {
        m_sem.wait();
        sylar::Mutex::Lock lock(m_mutex);
        auto v = m_datas.front();
        m_datas.pop_front();
        return v;
    }

    size_t size() {
        sylar::Mutex::Lock lock(m_mutex);
        return m_datas.size();
    }

    bool empty() {
        sylar::Mutex::Lock lock(m_mutex);
        return m_datas.empty();
    }
private:
    Semaphore m_sem;
    sylar::Mutex m_mutex;
    std::list<data_type> m_datas;
};

}
}

#endif
