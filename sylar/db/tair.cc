#include "tair.h"
#include "tair_client_api.hpp"
#include "sylar/util.h"
#include "sylar/log.h"
#include "sylar/config.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_tair =
    sylar::Config::Lookup("tair.config", std::map<std::string, std::map<std::string, std::string> >(), "tair config");

Tair::Tair()
    :m_client(nullptr)
    ,m_sn(0)
    ,m_timeout(2000)
    ,m_threadCount(4) {
    m_client = new tair::tair_client_api;
    m_logLevel = "error";
    m_logFile = "./tair.log";
}

Tair::~Tair() {
    if(m_client) {
        delete m_client;
    }
}

void Tair::setupCache(int area, size_t capacity, uint64_t expired_time) {
    sylar::Mutex::Lock lock(m_mutex);
    setupCacheNolock(area, capacity, expired_time);
}

void Tair::setupCacheNolock(int area, size_t capacity, uint64_t expired_time) {
    m_caches[area] = std::make_pair(capacity, expired_time);
    m_client->setup_cache(area, capacity, expired_time);
}

bool Tair::init(const std::map<std::string, std::string>& confs) {
    sylar::Mutex::Lock lock(m_mutex);
    m_confs = confs;
    m_threadCount = sylar::GetParamValue(m_confs, "thread_count", m_threadCount);
    m_timeout = sylar::GetParamValue(m_confs, "timeout", m_timeout);
    m_logLevel = sylar::GetParamValue(m_confs, "log_level", m_logLevel);
    m_logFile = sylar::GetParamValue(m_confs, "log_file", m_logFile);
    m_masterAddr = sylar::GetParamValue(m_confs, "master_addr", m_masterAddr);
    m_slaveAddr = sylar::GetParamValue(m_confs, "slave_addr", m_slaveAddr);
    m_groupName = sylar::GetParamValue(m_confs, "group_name", m_groupName);

    for(auto& i : m_confs) {
        if(i.first.find("cache.") != 0) {
            continue;
        }
        auto idx = sylar::TypeUtil::Atoi(i.first.substr(6));
        auto parts = sylar::split(i.second, ",");
        if(parts.size() != 2) {
            SYLAR_LOG_ERROR(g_logger) << "invalid cache config: " << i.first << " : " << i.second;
            continue;
        }
        size_t cap = sylar::TypeUtil::Atoi(parts[0]);
        size_t et = sylar::TypeUtil::Atoi(parts[1]);

        setupCacheNolock(idx, cap, et);
    }
    lock.unlock();
    return true;
}

const std::string Tair::toString() {
    std::stringstream ss;
    ss << "tair client: master_addr="
       << m_masterAddr
       << " slave_addr=" << m_slaveAddr
       << " group_name=" << m_groupName
       << " thread_count=" << m_threadCount
       << " log_level=" << m_logLevel
       << " log_file=" << m_logFile
       << " timeout=" << m_timeout
       << " cache=[";
    bool is_first = true;
    sylar::Mutex::Lock lock(m_mutex);
    for(auto& i : m_caches) {
        if(!is_first) {
            ss << ",";
        }
        ss << i.first << ":" << i.second.first << ":" << i.second.second;
        is_first = false;
    }
    ss << "]";
    return ss.str();
}

bool Tair::startup() {
    bool rt = false;
    m_client->set_log_level(m_logLevel.c_str());
    m_client->set_timeout(m_timeout);
    m_client->set_thread_count(m_threadCount);
    //m_client->set_force_service(true);
    rt = m_client->startup(m_masterAddr.c_str(), m_slaveAddr.c_str(), m_groupName.c_str());
    //m_client->set_log_file(m_logFile.c_str());
    if(rt) {
        std::set<uint64_t> servers;
        m_client->get_servers(servers);
        m_client->set_timeout(10);
        for(auto& i : servers) {
            if(!m_client->ping(i)) {
            }
        }
        m_client->set_timeout(m_timeout);
    } else {
        SYLAR_LOG_ERROR(g_logger) << "tair startup err: " << toString();
    }
    return rt;
}

bool Tair::startup(const char* master_addr, const char* slave_addr, const char* group_name) {
    m_masterAddr = master_addr;
    m_slaveAddr = slave_addr;
    m_groupName = group_name;
    return startup();
}

int32_t Tair::get(const std::string& key, std::string& val, int area, int timeout_ms) {
    tair::common::data_entry dbkey(key.c_str(), key.size(), false);
    auto sn = sylar::Atomic::addFetch(m_sn);
    Ctx::ptr c = std::make_shared<Ctx>();
    c->sn = sn;
    c->scheduler = sylar::Scheduler::GetThis();
    c->fiber = sylar::Fiber::GetThis();
    c->client = this;
    c->key = key;
    c->start = sylar::GetCurrentMS();
    do {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs[sn] = c;
    } while(0);

    Ctx::weak_ptr* wp(new Ctx::weak_ptr(c));
    int ret = m_client->async_get(area, dbkey, Tair::OnGetCb, wp);
    if(ret != TAIR_RETURN_SUCCESS) {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs.erase(sn);
        return ret;
    }

    if(timeout_ms == -1) {
        timeout_ms = m_timeout;
    }

    if(timeout_ms) {
        c->timer = sylar::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&Tair::onTimer, this, c));
    }
    sylar::Fiber::YieldToHold();
    val.swap(c->val);
    return c->result;
}

int32_t Tair::put(const std::string& key, const std::string& val, int area, int expired, int version) {
    tair::common::data_entry dbkey(key.c_str(), key.size(), false);
    tair::common::data_entry dbval(val.c_str(), val.size(), false);
    auto sn = sylar::Atomic::addFetch(m_sn);
    Ctx::ptr c = std::make_shared<Ctx>();
    c->sn = sn;
    c->scheduler = sylar::Scheduler::GetThis();
    c->fiber = sylar::Fiber::GetThis();
    c->key = key;
    c->client = this;
    c->start = sylar::GetCurrentMS();
    do {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs[sn] = c;
    } while(0);

    Ctx::weak_ptr* wp(new Ctx::weak_ptr(c));
    int ret = m_client->async_put(area, dbkey, dbval, expired, version, Tair::OnPutCb, wp);
    if(ret != TAIR_RETURN_SUCCESS) {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs.erase(sn);
        return ret;
    }
    sylar::Fiber::YieldToHold();
    return c->result;
}

int32_t Tair::remove(const std::string& key, int area) {
    tair::common::data_entry dbkey(key.c_str(), key.size(), false);
    auto sn = sylar::Atomic::addFetch(m_sn);
    Ctx::ptr c = std::make_shared<Ctx>();
    c->sn = sn;
    c->scheduler = sylar::Scheduler::GetThis();
    c->fiber = sylar::Fiber::GetThis();
    c->client = this;
    c->key = key;
    c->start = sylar::GetCurrentMS();
    do {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs[sn] = c;
    } while(0);

    Ctx::weak_ptr* wp(new Ctx::weak_ptr(c));
    int ret = m_client->async_remove(area, dbkey, Tair::OnRemoveCb, wp);
    if(ret != TAIR_RETURN_SUCCESS) {
        sylar::Mutex::Lock lock(m_mutex);
        m_ctxs.erase(sn);
        return ret;
    }
    sylar::Fiber::YieldToHold();
    return c->result;
}


void Tair::OnGetCb(int result, const tair::common::data_entry *key, const tair::common::data_entry *value, void *args) {
    if(result == TAIR_RETURN_DATA_NOT_EXIST) {
        result = 0;
    }
    Ctx::weak_ptr* wp = (Ctx::weak_ptr*)args;
    auto ctx = wp->lock();
    delete wp;
    if(!ctx) {
        if(result) {
            SYLAR_LOG_ERROR(g_logger) << "OnGetCb result=" << result
                << " key=" << (key ? key->get_data() : "[null]");
        } else {
            SYLAR_LOG_INFO(g_logger) << "OnGetCb result=" << result
                << " key=" << (key ? key->get_data() : "[null]");
        }
        return;
    }

    if(result) {
        SYLAR_LOG_ERROR(g_logger) << "OnGetCb result=" << result
            << " key=" << ctx->key
            << " sn=" << ctx->sn
            << " used=" << (sylar::GetCurrentMS() - ctx->start);
    }

    do {
        sylar::Mutex::Lock lock(ctx->client->m_mutex);
        auto it = ctx->client->m_ctxs.find(ctx->sn);
        if(it == ctx->client->m_ctxs.end()) {
            return;
        }
        ctx->client->m_ctxs.erase(it);
    } while(0);

    if(value) {
        ctx->val.append(value->get_data(), value->get_size());
    }
    if(ctx->timer) {
        ctx->timer->cancel();
        ctx->timer = nullptr;
    }
    ctx->result = result;
    ctx->scheduler->schedule(&ctx->fiber);
}

void Tair::OnRemoveCb(int result, void *args) {
    Ctx::weak_ptr* wp = (Ctx::weak_ptr*)args;
    auto ctx = wp->lock();
    delete wp;
    if(!ctx) {
        if(result) {
            SYLAR_LOG_ERROR(g_logger) << "OnRemoveCb result=" << result;
        } else {
            SYLAR_LOG_INFO(g_logger) << "OnRemoveCb result=" << result;
        }
        return;
    }

    do {
        sylar::Mutex::Lock lock(ctx->client->m_mutex);
        ctx->client->m_ctxs.erase(ctx->sn);
    } while(0);

    if(result) {
            SYLAR_LOG_ERROR(g_logger) << "OnRemoveCb result=" << result
                << " key=" << ctx->key
                << " sn=" << ctx->sn;
    }

    if(ctx->timer) {
        ctx->timer->cancel();
        ctx->timer = nullptr;
    }
    ctx->result = result;
    ctx->scheduler->schedule(&ctx->fiber);
}

void Tair::OnPutCb(int result, void *args) {
    Ctx::weak_ptr* wp = (Ctx::weak_ptr*)args;
    auto ctx = wp->lock();
    delete wp;
    if(!ctx) {
        if(result) {
            SYLAR_LOG_ERROR(g_logger) << "OnPutCb result=" << result;
        } else {
            SYLAR_LOG_INFO(g_logger) << "OnPutCb result=" << result;
        }
        return;
    }

    do {
        sylar::Mutex::Lock lock(ctx->client->m_mutex);
        ctx->client->m_ctxs.erase(ctx->sn);
    } while(0);

    if(result) {
            SYLAR_LOG_ERROR(g_logger) << "OnPutCb result=" << result
                << " key=" << ctx->key
                << " sn=" << ctx->sn
                << " used=" << (sylar::GetCurrentMS() - ctx->start);
    }

    if(ctx->timer) {
        ctx->timer->cancel();
        ctx->timer = nullptr;
    }
    ctx->result = result;
    ctx->scheduler->schedule(&ctx->fiber);

}

void Tair::onTimer(Ctx::ptr ctx) {
    SYLAR_LOG_ERROR(g_logger) << "onTimer sn=" << ctx->sn
        << " key=" << ctx->key
        << " used=" << (sylar::GetCurrentMS() - ctx->start);
    sylar::Mutex::Lock lock(m_mutex);
    if(ctx->fiber) {
        auto it = m_ctxs.find(ctx->sn);
        if(it == m_ctxs.end()) {
            return;
        }
        m_ctxs.erase(it);
        lock.unlock();
        //SYLAR_LOG_ERROR(g_logger) << "onTimer sn=" << ctx->sn;
        ctx->result = TAIR_RETURN_TIMEOUT;
        ctx->scheduler->schedule(&ctx->fiber);
    }
}

std::string Tair::Ctx::toString() const {
    std::stringstream ss;
    ss << "[ctx"
       << " result=" << result
       << " key=" << key
       << " val=" << val
       << " sn=" << sn
       << "]";
    return ss.str();
}

TairManager::TairManager()
    :m_idx(0) {
    init();
}

Tair::ptr TairManager::get(const std::string& name) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.empty()) {
        return nullptr;
    }
    return it->second[sylar::Atomic::addFetch(m_idx) % it->second.size()];
}

std::ostream& TairManager::dump(std::ostream& os) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    os << "[TairManager total=" << m_config.size() << "]" << std::endl;
    for(auto& i : m_config) {
        os << "    " << i.first << " :[";
        for(auto& n : i.second) {
            os << "{" << n.first << ":" << n.second << "}";
        }
        os << "]" << std::endl;
    }
    return os;
}

int32_t TairUtil::Get(const std::string& name, const std::string& key, std::string& val, int area, int timeout_ms) {
    auto tair = TairMgr::GetInstance()->get(name);
    if(!tair) {
        return TAIR_RETURN_INVAL_CONN_ERROR;
    }
    return tair->get(key, val, area, timeout_ms);
}

int32_t TairUtil::Put(const std::string& name, const std::string& key, const std::string& val, int area, int expired, int version) {
    auto tair = TairMgr::GetInstance()->get(name);
    if(!tair) {
        return TAIR_RETURN_INVAL_CONN_ERROR;
    }
    return tair->put(key, val, area, expired, version);
}

int32_t TairUtil::Remove(const std::string& name, const std::string& key, int area) {
    auto tair = TairMgr::GetInstance()->get(name);
    if(!tair) {
        return TAIR_RETURN_INVAL_CONN_ERROR;
    }
    return tair->remove(key, area);
}

int32_t TairUtil::TryGet(const std::string& name, uint32_t try_count, const std::string& key, std::string& val, int area, int timeout_ms) {
    int32_t rt = -1;
    for(uint32_t i = 0; i < try_count; ++i) {
        rt = Get(name, key, val, area, timeout_ms);
        if(!rt) {
            break;
        }
    }
    return rt;
}

int32_t TairUtil::TryPut(const std::string& name, uint32_t try_count, const std::string& key, const std::string& val, int area, int expired, int version) {
    int32_t rt = -1;
    for(uint32_t i = 0; i < try_count; ++i) {
        rt = Put(name, key, val, area, expired, version);
        if(!rt) {
            break;
        }
    }
    return rt;
}

int32_t TairUtil::TryRemove(const std::string& name, uint32_t try_count, const std::string& key, int area) {
    int32_t rt = -1;
    for(uint32_t i = 0; i < try_count; ++i) {
        rt = Remove(name, key, area);
        if(!rt) {
            break;
        }
    }
    return rt;
}

void TairManager::init() {
    m_config = g_tair->getValue();
    size_t done = 0;
    size_t total = 0;
    for(auto& i : m_config) {
        int pool = sylar::GetParamValue(i.second, "pool", 1);
        total += pool;
        for(int n = 0; n < pool; ++n) {
            sylar::Tair::ptr t = std::make_shared<sylar::Tair>();
            t->init(i.second);
            if(!t->startup()) {
                SYLAR_LOG_ERROR(g_logger) << "tair name=" << i.first << " startup fail";
            } else {
                sylar::RWMutex::WriteLock lock(m_mutex);
                m_datas[i.first].push_back(t);
            }
            sylar::Atomic::addFetch(done, 1);
        }
    }

    while(done != total) {
        usleep(5000);
    }
}

}
