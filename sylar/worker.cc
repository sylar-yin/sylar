#include "worker.h"
#include "config.h"
#include "util.h"

namespace sylar {

static sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_worker_config
    = sylar::Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string> >(), "worker config");

WorkerGroup::WorkerGroup(uint32_t batch_size, sylar::Scheduler* s)
    :m_batchSize(batch_size)
    ,m_finish(false)
    ,m_scheduler(s)
    ,m_sem(batch_size) {
}

WorkerGroup::~WorkerGroup() {
    waitAll();
}

void WorkerGroup::schedule(std::function<void()> cb, int thread) {
    m_sem.wait();
    m_scheduler->schedule(std::bind(&WorkerGroup::doWork
                          ,shared_from_this(), cb), thread);
}

void WorkerGroup::doWork(std::function<void()> cb) {
    cb();
    m_sem.notify();
}

void WorkerGroup::waitAll() {
    if(!m_finish) {
        m_finish = true;
        for(uint32_t i = 0; i < m_batchSize; ++i) {
            m_sem.wait();
        }
    }
}

TimedWorkerGroup::ptr TimedWorkerGroup::Create(uint32_t batch_size, uint32_t wait_ms, sylar::IOManager* s) {
    auto rt = std::make_shared<TimedWorkerGroup>(batch_size, wait_ms, s);
    rt->start();
    return rt;
}

void TimedWorkerGroup::start() {
    m_timer = m_iomanager->addTimer(m_waitTime, std::bind(&TimedWorkerGroup::onTimer, shared_from_this()));
}

void TimedWorkerGroup::onTimer() {
    m_timedout = true;
    m_sem.notifyAll();
    m_timer = nullptr;
}

TimedWorkerGroup::TimedWorkerGroup(uint32_t batch_size, uint32_t wait_ms, sylar::IOManager* s)
    :m_batchSize(batch_size)
    ,m_finish(false)
    ,m_timedout(false)
    ,m_waitTime(wait_ms)
    ,m_iomanager(s)
    ,m_sem(batch_size) {
}

TimedWorkerGroup::~TimedWorkerGroup() {
    std::cout << "====TimedWorkerGroup::~TimedWorkerGroup====" << std::endl;
}

void TimedWorkerGroup::schedule(std::function<void()> cb, int thread) {
    if(!m_timedout) {
        m_sem.wait();
    }
    m_iomanager->schedule(std::bind(&TimedWorkerGroup::doWork
                          ,shared_from_this(), cb), thread);
}

void TimedWorkerGroup::doWork(std::function<void()> cb) {
    cb();
    m_sem.notify();
}

void TimedWorkerGroup::waitAll() {
    if(!m_finish) {
        m_finish = true;
        for(uint32_t i = 0; i < m_batchSize && !m_timedout; ++i) {
            m_sem.wait();
        }

        if(!m_timedout && m_timer) {
            m_timer->cancel();
            m_timer = nullptr;
        }
    }
}

WorkerManager::WorkerManager()
    :m_stop(false) {
}

void WorkerManager::add(Scheduler::ptr s) {
    m_datas[s->getName()].push_back(s);
}

void WorkerManager::add(const std::string& name, Scheduler::ptr s) {
    m_datas[name].push_back(s);
}

Scheduler::ptr WorkerManager::get(const std::string& name) {
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.size() == 1) {
        return it->second[0];
    }
    static uint32_t s_count = 0;
    return it->second[sylar::Atomic::addFetch(s_count) % it->second.size()];
}

IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string> >& v) {
    for(auto& i : v) {
        std::string name = i.first;
        int32_t thread_num = sylar::GetParamValue(i.second, "thread_num", 1);
        int32_t worker_num = sylar::GetParamValue(i.second, "worker_num", 1);

        for(int32_t x = 0; x < worker_num; ++x) {
            Scheduler::ptr s;
            if(!x) {
                s = std::make_shared<IOManager>(thread_num, false, name);
            } else {
                s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(x));
            }
            add(name, s);
        }
    }
    m_stop = m_datas.empty();
    return true;
}

bool WorkerManager::init() {
    auto workers = g_worker_config->getValue();
    return init(workers);
}

void WorkerManager::stop() {
    if(m_stop) {
        return;
    }
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->schedule([](){});
            n->stop();
        }
    }
    m_datas.clear();
    m_stop = true;
}

uint32_t WorkerManager::getCount() {
    return m_datas.size();
}

std::ostream& WorkerManager::dump(std::ostream& os) {
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->dump(os) << std::endl;
        }
    }
    return os;
}

}
