#include "fox_thread.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/util.h"
#include "sylar/macro.h"
#include "sylar/config.h"
#include <iomanip>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_thread_info_set
            = Config::Lookup("fox_thread", std::map<std::string, std::map<std::string, std::string> >()
                    ,"confg for thread");

static RWMutex s_thread_mutex;
static std::map<uint64_t, std::string> s_thread_names;

thread_local FoxThread* s_thread = nullptr;

void FoxThread::read_cb(evutil_socket_t sock, short which, void* args) {
    FoxThread* thread = static_cast<FoxThread*>(args);
    uint8_t cmd[4096];
    if(recv(sock, cmd, sizeof(cmd), 0) > 0) {
        std::list<callback> callbacks;
        RWMutex::WriteLock lock(thread->m_mutex);
        callbacks.swap(thread->m_callbacks);
        lock.unlock();
        thread->m_working = true;
        for(auto it = callbacks.begin();
                it != callbacks.end(); ++it) {
            if(*it) {
                //SYLAR_ASSERT(thread == GetThis());
                try {
                    (*it)();
                } catch (std::exception& ex) {
                    SYLAR_LOG_ERROR(g_logger) << "exception:" << ex.what();
                } catch (const char* c) {
                    SYLAR_LOG_ERROR(g_logger) << "exception:" << c;
                } catch (...) {
                    SYLAR_LOG_ERROR(g_logger) << "uncatch exception";
                }
            } else {
                event_base_loopbreak(thread->m_base);
                thread->m_start = false;
                thread->unsetThis();
                break;
            }
        }
        sylar::Atomic::addFetch(thread->m_total, callbacks.size());
        thread->m_working = false;
    }
}

FoxThread::FoxThread(const std::string& name, struct event_base* base)
    :m_read(0)
    ,m_write(0)
    ,m_base(NULL)
    ,m_event(NULL)
    ,m_thread(NULL)
    ,m_name(name)
    ,m_working(false)
    ,m_start(false)
    ,m_total(0) {
    int fds[2];
    if(evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
        //SYLAR_LOG_ERROR(g_logger) << "FoxThread init error";
        throw std::logic_error("thread init error");
    }

    evutil_make_socket_nonblocking(fds[0]);
    evutil_make_socket_nonblocking(fds[1]);

    m_read = fds[0];
    m_write = fds[1];

    if(base) {
        m_base = base;
        setThis();
    } else {
        m_base = event_base_new();
    }
    m_event = event_new(m_base, m_read, EV_READ | EV_PERSIST, read_cb, this);
    event_add(m_event, NULL);
}

void FoxThread::dump(std::ostream& os) {
    RWMutex::ReadLock lock(m_mutex);
    os << "[thread name=" << m_name
       << " working=" << m_working
       << " tasks=" << m_callbacks.size()
       << " total=" << m_total
       << "]" << std::endl;
}

std::thread::id FoxThread::getId() const {
    if(m_thread) {
        return m_thread->get_id();
    }
    return std::thread::id();
}

void* FoxThread::getData(const std::string& name) {
    //Mutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(name);
    return it == m_datas.end() ? nullptr : it->second;
}

void  FoxThread::setData(const std::string& name, void* v) {
    //Mutex::WriteLock lock(m_mutex);
    m_datas[name] = v;
}

FoxThread::~FoxThread() {
    if(m_read) {
        close(m_read);
    }
    if(m_write) {
        close(m_write);
    }
    stop();
    join();
    if(m_thread) {
        delete m_thread;
    }
    if(m_event) {
        event_free(m_event);
    }
    if(m_base) {
        event_base_free(m_base);
    }
}

void FoxThread::start() {
    if(m_thread) {
        //SYLAR_LOG_ERROR(g_logger) << "FoxThread is running";
        throw std::logic_error("FoxThread is running");
    }

    m_thread = new std::thread(std::bind(&FoxThread::thread_cb, this));
    m_start = true;
}

void FoxThread::thread_cb() {
    //std::cout << "FoxThread(" << m_name << "," << pthread_self() << ")" << std::endl;
    setThis();
    pthread_setname_np(pthread_self(), m_name.substr(0, 15).c_str());
    if(m_initCb) {
        m_initCb(this);
        m_initCb = nullptr;
    }
    event_base_loop(m_base, 0);
}

bool FoxThread::dispatch(callback cb) {
    RWMutex::WriteLock lock(m_mutex);
    m_callbacks.push_back(cb);
    //if(m_callbacks.size() > 1) {
    //    std::cout << std::this_thread::get_id() << ":" << m_callbacks.size() << " " << m_name << std::endl;
    //}
    lock.unlock();
    uint8_t cmd = 1;
    //write(m_write, &cmd, sizeof(cmd));
    if(send(m_write, &cmd, sizeof(cmd), 0) <= 0) {
        return false;
    }
    return true;
}

bool FoxThread::dispatch(uint32_t id, callback cb) {
    return dispatch(cb);
}

bool FoxThread::batchDispatch(const std::vector<callback>& cbs) {
    RWMutex::WriteLock lock(m_mutex);
    for(auto& i : cbs) {
        m_callbacks.push_back(i);
    }
    lock.unlock();
    uint8_t cmd = 1;
    if(send(m_write, &cmd, sizeof(cmd), 0) <= 0) {
        return false;
    }
    return true;
}

void FoxThread::broadcast(callback cb) {
    dispatch(cb);
}

void FoxThread::stop() {
    RWMutex::WriteLock lock(m_mutex);
    m_callbacks.push_back(nullptr);
    if(m_thread) {
        uint8_t cmd = 0;
        //write(m_write, &cmd, sizeof(cmd));
        send(m_write, &cmd, sizeof(cmd), 0);
    }
    //if(m_data) {
    //    delete m_data;
    //    m_data = NULL;
    //}
}

void FoxThread::join() {
    if(m_thread) {
        m_thread->join();
        delete m_thread;
        m_thread = NULL;
    }
}

FoxThreadPool::FoxThreadPool(uint32_t size, const std::string& name, bool advance)
    :m_size(size)
    ,m_cur(0)
    ,m_name(name)
    ,m_advance(advance)
    ,m_start(false)
    ,m_total(0) {
    m_threads.resize(m_size);
    for(size_t i = 0; i < size; ++i) {
        FoxThread* t(new FoxThread(name + "_" + std::to_string(i)));
        m_threads[i] = t;
    }
}

FoxThreadPool::~FoxThreadPool() {
    for(size_t i = 0; i < m_size; ++i) {
        delete m_threads[i];
    }
}

void FoxThreadPool::start() {
    for(size_t i = 0; i < m_size; ++i) {
        m_threads[i]->setInitCb(m_initCb);
        m_threads[i]->start();
        m_freeFoxThreads.push_back(m_threads[i]);
    }
    if(m_initCb) {
        m_initCb = nullptr;
    }
    m_start = true;
    check();
}

void FoxThreadPool::stop() {
    for(size_t i = 0; i < m_size; ++i) {
        m_threads[i]->stop();
    }
    m_start = false;
}

void FoxThreadPool::join() {
    for(size_t i = 0; i < m_size; ++i) {
        m_threads[i]->join();
    }
}

void FoxThreadPool::releaseFoxThread(FoxThread* t) {
    do {
        RWMutex::WriteLock lock(m_mutex);
        m_freeFoxThreads.push_back(t);
    } while(0);
    check();
}

bool FoxThreadPool::dispatch(callback cb) {
    do {
        sylar::Atomic::addFetch(m_total, (uint64_t)1);
        RWMutex::WriteLock lock(m_mutex);
        if(!m_advance) {
            return m_threads[m_cur++ % m_size]->dispatch(cb);
        }
        m_callbacks.push_back(cb);
    } while(0);
    check();
    return true;
}

bool FoxThreadPool::batchDispatch(const std::vector<callback>& cbs) {
    sylar::Atomic::addFetch(m_total, cbs.size());
    RWMutex::WriteLock lock(m_mutex);
    if(!m_advance) {
        for(auto cb : cbs) {
            m_threads[m_cur++ % m_size]->dispatch(cb);
        }
        return true;
    }
    for(auto cb : cbs) {
        m_callbacks.push_back(cb);
    }
    lock.unlock();
    check();
    return true;
}

void FoxThreadPool::check() {
    do {
        if(!m_start) {
            break;
        }
        RWMutex::WriteLock lock(m_mutex);
        if(m_freeFoxThreads.empty() || m_callbacks.empty()) {
            break;
        }

        std::shared_ptr<FoxThread> thr(m_freeFoxThreads.front(),
                std::bind(&FoxThreadPool::releaseFoxThread,
                    this, std::placeholders::_1));
        m_freeFoxThreads.pop_front();

        callback cb = m_callbacks.front();
        m_callbacks.pop_front();
        lock.unlock();

        if(thr->isStart()) {
            thr->dispatch(std::bind(&FoxThreadPool::wrapcb, this, thr, cb));
        } else {
            RWMutex::WriteLock lock(m_mutex);
            m_callbacks.push_front(cb);
        }
    } while(true);
}

void FoxThreadPool::wrapcb(std::shared_ptr<FoxThread> thr, callback cb) {
    cb();
}

bool FoxThreadPool::dispatch(uint32_t id, callback cb) {
    sylar::Atomic::addFetch(m_total, (uint64_t)1);
    return m_threads[id % m_size]->dispatch(cb);
}

FoxThread* FoxThreadPool::getRandFoxThread() {
    return m_threads[m_cur++ % m_size];
}

void FoxThreadPool::broadcast(callback cb) {
    for(size_t i = 0; i < m_threads.size(); ++i) {
        m_threads[i]->dispatch(cb);
    }
}

void FoxThreadPool::dump(std::ostream& os) {
    RWMutex::ReadLock lock(m_mutex);
    os << "[FoxThreadPool name = " << m_name << " thread_count = " << m_threads.size()
       << " tasks = " << m_callbacks.size() << " total = " << m_total
       << " advance = " << m_advance
       << "]" << std::endl;
    for(size_t i = 0; i < m_threads.size(); ++i) {
        os << "    ";
        m_threads[i]->dump(os);
    }
}

FoxThread* FoxThread::GetThis() {
    return s_thread;
}

const std::string& FoxThread::GetFoxThreadName() {
    FoxThread* t = GetThis();
    if(t) {
        return t->m_name;
    }

    uint64_t tid = sylar::GetThreadId();
    do {
        RWMutex::ReadLock lock(s_thread_mutex);
        auto it = s_thread_names.find(tid);
        if(it != s_thread_names.end()) {
            return it->second;
        }
    } while(0);

    do {
        RWMutex::WriteLock lock(s_thread_mutex);
        s_thread_names[tid] = "UNNAME_" + std::to_string(tid);
        return s_thread_names[tid];
    } while (0);
}

void FoxThread::GetAllFoxThreadName(std::map<uint64_t, std::string>& names) {
    RWMutex::ReadLock lock(s_thread_mutex);
    for(auto it = s_thread_names.begin();
            it != s_thread_names.end(); ++it) {
        names.insert(*it);
    }
}

void FoxThread::setThis() {
    m_name = m_name + "_" + std::to_string(sylar::GetThreadId());
    s_thread = this;

    RWMutex::WriteLock lock(s_thread_mutex);
    s_thread_names[sylar::GetThreadId()] = m_name;
}

void FoxThread::unsetThis() {
    s_thread = nullptr;
    RWMutex::WriteLock lock(s_thread_mutex);
    s_thread_names.erase(sylar::GetThreadId());
}

IFoxThread::ptr FoxThreadManager::get(const std::string& name) {
    auto it = m_threads.find(name);
    return it == m_threads.end() ? nullptr : it->second;
}

void FoxThreadManager::add(const std::string& name, IFoxThread::ptr thr) {
    m_threads[name] = thr;
}

void FoxThreadManager::dispatch(const std::string& name, callback cb) {
    IFoxThread::ptr ti = get(name);
    SYLAR_ASSERT(ti);
    ti->dispatch(cb);
}

void FoxThreadManager::dispatch(const std::string& name, uint32_t id, callback cb) {
    IFoxThread::ptr ti = get(name);
    SYLAR_ASSERT(ti);
    ti->dispatch(id, cb);
}

void FoxThreadManager::batchDispatch(const std::string& name, const std::vector<callback>& cbs) {
    IFoxThread::ptr ti = get(name);
    SYLAR_ASSERT(ti);
    ti->batchDispatch(cbs);
}

void FoxThreadManager::broadcast(const std::string& name, callback cb) {
    IFoxThread::ptr ti = get(name);
    SYLAR_ASSERT(ti);
    ti->broadcast(cb);
}

void FoxThreadManager::dumpFoxThreadStatus(std::ostream& os) {
    os << "FoxThreadManager: " << std::endl;
    for(auto it = m_threads.begin();
            it != m_threads.end(); ++it) {
        it->second->dump(os);
    }

    os << "All FoxThreads:" << std::endl;
    std::map<uint64_t, std::string> names;
    FoxThread::GetAllFoxThreadName(names);
    for(auto it = names.begin();
            it != names.end(); ++it) {
        os << std::setw(30) << it->first
           << ": " << it->second << std::endl;
    }
}

void FoxThreadManager::init() {
    auto m = g_thread_info_set->getValue();
    for(auto i : m) {
        auto num = sylar::GetParamValue(i.second, "num", 0);
        auto name = i.first;
        auto advance = sylar::GetParamValue(i.second, "advance", 0);
        if(num <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "thread pool:" << name
                        << " num:" << num
                        << " advance:" << advance
                        << " invalid";
            continue;
        }
        if(num == 1) {
            m_threads[i.first] = FoxThread::ptr(new FoxThread(i.first));
            SYLAR_LOG_INFO(g_logger) << "init thread : " << i.first;
        } else {
            m_threads[i.first] = FoxThreadPool::ptr(new FoxThreadPool(
                            num, name, advance));
            SYLAR_LOG_INFO(g_logger) << "init thread pool:" << name
                       << " num:" << num
                       << " advance:" << advance;
        }
    }
}

void FoxThreadManager::start() {
    for(auto i : m_threads) {
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " start begin";
        i.second->start();
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " start end";
    }
}

void FoxThreadManager::stop() {
    for(auto i : m_threads) {
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " stop begin";
        i.second->stop();
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " stop end";
    }
    for(auto i : m_threads) {
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " join begin";
        i.second->join();
        SYLAR_LOG_INFO(g_logger) << "thread: " << i.first << " join end";
    }
}

}
