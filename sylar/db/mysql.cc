#include "mysql.h"
#include "sylar/log.h"
#include "sylar/config.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<std::map<std::string, std::map<std::string, std::string> > >::ptr g_mysql_dbs
    = sylar::Config::Lookup("mysql.dbs", std::map<std::string, std::map<std::string, std::string> >()
            , "mysql dbs");

namespace {

    struct MySQLThreadIniter {
        MySQLThreadIniter() {
            mysql_thread_init();
        }

        ~MySQLThreadIniter() {
            mysql_thread_end();
        }
    };
}

static MYSQL* mysql_init(std::map<std::string, std::string>& params,
                         const int& timeout) {

    static thread_local MySQLThreadIniter s_thread_initer;

    MYSQL* mysql = ::mysql_init(nullptr);
    if(mysql == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_init error";
        return nullptr;
    }

    if(timeout > 0) {
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    }
    bool close = false;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &close);
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "UTF8");

    int port = sylar::GetParamValue(params, "port", 0);
    std::string host = sylar::GetParamValue<std::string>(params, "host");
    std::string user = sylar::GetParamValue<std::string>(params, "user");
    std::string passwd = sylar::GetParamValue<std::string>(params, "passwd");
    std::string dbname = sylar::GetParamValue<std::string>(params, "dbname");

    if(mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str()
                          ,dbname.c_str(), port, NULL, 0) == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_real_connect(" << host
                                  << ", " << port << ", " << dbname
                                  << ") error: " << mysql_error(mysql);
        mysql_close(mysql);
        return nullptr;
    }
    return mysql;
}

MySQL::MySQL(const std::map<std::string, std::string>& args)
    :m_params(args)
    ,m_lastUsedTime(0)
    ,m_hasError(false) {
}

bool MySQL::connect() {
    if(m_mysql && !m_hasError) {
        return true;
    }

    MYSQL* m = mysql_init(m_params, 0);
    if(!m) {
        m_hasError = true;
        return false;
    }
    m_hasError = false;
    m_mysql.reset(m, mysql_close);
    return true;
}

bool MySQL::isNeedCheck() {
    if((time(0) - m_lastUsedTime) < 5
            && !m_hasError) {
        return false;
    }
    return true;
}

bool MySQL::ping() {
    if(!m_mysql) {
        return false;
    }
    if(mysql_ping(m_mysql.get())) {
        m_hasError = true;
        return false;
    }
    m_hasError = false;
    return true;
}

int MySQL::cmd(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = cmd(format, ap);
    va_end(ap);
    return rt;
}

int MySQL::cmd(const char* format, va_list ap) {
    m_cmd = sylar::StringUtil::Formatv(format, ap);
    int r = ::mysql_query(m_mysql.get(), m_cmd.c_str());
    if(r) {
        SYLAR_LOG_ERROR(g_logger) << "cmd=" << cmd()
            << ", error: " << getError();
        m_hasError = true;
    } else {
        m_hasError = false;
    }
    return r;
}

int MySQL::cmd(const std::string& sql) {
    m_cmd = sql;
    int r = ::mysql_query(m_mysql.get(), m_cmd.c_str());
    if(r) {
        SYLAR_LOG_ERROR(g_logger) << "cmd=" << cmd()
            << ", error: " << getError();
        m_hasError = true;
    } else {
        m_hasError = false;
    }
    return r;
}

std::shared_ptr<MySQL> MySQL::getMySQL() {
    return MySQL::ptr(this, sylar::nop<MySQL>);
}

uint64_t MySQL::getAffectedRows() {
    if(!m_mysql) {
        return 0;
    }
    return mysql_affected_rows(m_mysql.get());
}

static MYSQL_RES* my_mysql_query(MYSQL* mysql, const char* sql) {
    if(mysql == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_query mysql is null";
        return nullptr;
    }

    if(sql == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_query sql is null";
        return nullptr;
    }

    if(::mysql_query(mysql, sql)) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_query(" << sql << ") error:"
            << mysql_error(mysql);
        return nullptr;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if(res == nullptr) {
        SYLAR_LOG_ERROR(g_logger) << "mysql_store_result() error:"
            << mysql_error(mysql);
    }
    return res;
}

MySQLRes::MySQLRes(MYSQL_RES* res, int eno, const char* estr)
    :m_errno(eno)
    ,m_errstr(estr) {
    if(res) {
        m_data.reset(res, mysql_free_result);
    }
}

uint64_t MySQLRes::getRows() {
    return mysql_num_rows(m_data.get());
}

uint64_t MySQLRes::getFields() {
    return mysql_num_fields(m_data.get());
}

bool MySQLRes::foreach(data_cb cb) {
    MYSQL_ROW row;
    uint64_t fields = getFields();
    int i = 0;
    while((row = mysql_fetch_row(m_data.get()))) {
        if(!cb(row, fields, i++)) {
            break;
        }
    }
    return true;
}

MySQLRes::ptr MySQL::query(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    auto rt = query(format, ap);
    va_end(ap);
    return rt;
}

MySQLRes::ptr MySQL::query(const char* format, va_list ap) {
    m_cmd = sylar::StringUtil::Formatv(format, ap);
    MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
    if(!res) {
        m_hasError = true;
        return nullptr;
    }
    m_hasError = false;
    MySQLRes::ptr rt(new MySQLRes(res, mysql_errno(m_mysql.get())
                        ,mysql_error(m_mysql.get())));
    return rt;
}

MySQLRes::ptr MySQL::query(const std::string& sql) {
    m_cmd = sql;
    MYSQL_RES* res = my_mysql_query(m_mysql.get(), m_cmd.c_str());
    if(!res) {
        m_hasError = true;
        return nullptr;
    }
    m_hasError = false;
    MySQLRes::ptr rt(new MySQLRes(res, mysql_errno(m_mysql.get())
                        ,mysql_error(m_mysql.get())));
    return rt;

}

const char* MySQL::cmd() {
    return m_cmd.c_str();
}

bool MySQL::use(const std::string& dbname) {
    if(!m_mysql) {
        return false;
    }
    if(m_dbname == dbname) {
        return true;
    }
    if(mysql_select_db(m_mysql.get(), dbname.c_str()) == 0) {
        m_dbname = dbname;
        m_hasError = false;
        return true;
    } else {
        m_dbname = "";
        m_hasError = true;
        return false;
    }
}

const char* MySQL::getError() {
    if(!m_mysql) {
        return "";
    }
    const char* str = mysql_error(m_mysql.get());
    if(str) {
        return str;
    }
    return "";
}

uint64_t MySQL::getInsertId() {
    if(m_mysql) {
        return mysql_insert_id(m_mysql.get());
    }
    return 0;
}

MySQLTransaction::ptr MySQLTransaction::Create(MySQL::ptr mysql, bool auto_commit) {
    MySQLTransaction::ptr rt(new MySQLTransaction(mysql, auto_commit));
    if(rt->cmd("BEGIN") == 0) {
        return rt;
    }
    return nullptr;
}

MySQLTransaction::~MySQLTransaction() {
    if(m_autoCommit) {
        commit();
    } else {
        rollback();
    }
}

bool MySQLTransaction::commit() {
    if(m_isFinished || m_hasError) {
        return !m_hasError;
    }
    int rt = cmd("COMMIT");
    if(rt == 0) {
        m_isFinished = true;
    } else {
        m_hasError = true;
    }
    return rt == 0;
}

bool MySQLTransaction::rollback() {
    if(m_isFinished) {
        return true;
    }
    int rt = cmd("ROLLBACK");
    if(rt == 0) {
        m_isFinished = true;
    } else {
        m_hasError = true;
    }
    return rt == 0;
}

int MySQLTransaction::cmd(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    return cmd(format, ap);
}

int MySQLTransaction::cmd(const char* format, va_list ap) {
    if(m_isFinished) {
        SYLAR_LOG_ERROR(g_logger) << "transaction is finished, format=" << format;
        return -1;
    }
    int rt = m_mysql->cmd(format, ap);
    if(rt) {
        m_hasError = true;
    }
    return rt;
}

int MySQLTransaction::cmd(const std::string& sql) {
    if(m_isFinished) {
        SYLAR_LOG_ERROR(g_logger) << "transaction is finished, sql=" << sql;
        return -1;
    }
    int rt = m_mysql->cmd(sql);
    if(rt) {
        m_hasError = true;
    }
    return rt;

}

std::shared_ptr<MySQL> MySQLTransaction::getMySQL() {
    return m_mysql;
}

MySQLTransaction::MySQLTransaction(MySQL::ptr mysql, bool auto_commit)
    :m_mysql(mysql)
    ,m_autoCommit(auto_commit) {
}

MySQLManager::MySQLManager()
    :m_maxConn(10) {
    mysql_library_init(0, nullptr, nullptr);
}

MySQLManager::~MySQLManager() {
    mysql_library_end();
}

MySQL::ptr MySQLManager::get(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_conns.find(name);
    if(it == m_conns.end()) {
        if(!it->second.empty()) {
            MySQL* rt = it->second.front();
            it->second.pop_front();
            lock.unlock();
            if(!rt->isNeedCheck()) {
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL,
                            this, name, std::placeholders::_1));
            }
            if(rt->ping()) {
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL,
                            this, name, std::placeholders::_1));
            } else if(rt->connect()) {
                rt->m_lastUsedTime = time(0);
                return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL,
                            this, name, std::placeholders::_1));
            } else {
                SYLAR_LOG_WARN(g_logger) << "reconnect " << name << " fail";
                return nullptr;
            }
        }
    }
    auto config = g_mysql_dbs->getValue();
    auto sit = config.find(name);
    std::map<std::string, std::string> args;
    if(sit != config.end()) {
        args = sit->second;
    } else {
        sit = m_dbDefines.find(name);
        if(sit != m_dbDefines.end()) {
            args = sit->second;
        } else {
            return nullptr;
        }
    }
    lock.unlock();
    MySQL* rt = new MySQL(args);
    if(rt->connect()) {
        rt->m_lastUsedTime = time(0);
        return MySQL::ptr(rt, std::bind(&MySQLManager::freeMySQL,
                    this, name, std::placeholders::_1));
    } else {
        delete rt;
        return nullptr;
    }
}

void MySQLManager::registerMySQL(const std::string& name, const std::map<std::string, std::string>& params) {
    MutexType::Lock lock(m_mutex);
    m_dbDefines[name] = params;
}

void MySQLManager::checkConnection(int sec) {
    time_t now = time(0);
    MutexType::Lock lock(m_mutex);
    for(auto& i : m_conns) {
        for(auto it = i.second.begin();
                it != i.second.end();) {
            if((int)(now - (*it)->m_lastUsedTime) >= sec) {
                auto tmp = *it;
                i.second.erase(it++);
                delete tmp;
            } else {
                ++it;
            }
        }
    }
}

int MySQLManager::cmd(const std::string& name, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int rt = cmd(name, format, ap);
    va_end(ap);
    return rt;
}

int MySQLManager::cmd(const std::string& name, const char* format, va_list ap) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "MySQLManager::cmd, get(" << name
            << ") fail, format=" << format;
        return -1;
    }
    return conn->cmd(format, ap);
}

int MySQLManager::cmd(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "MySQLManager::cmd, get(" << name
            << ") fail, sql=" << sql;
        return -1;
    }
    return conn->cmd(sql);
}

MySQLRes::ptr MySQLManager::query(const std::string& name, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    auto res = query(name, format, ap);
    va_end(ap);
    return res;
}

MySQLRes::ptr MySQLManager::query(const std::string& name, const char* format, va_list ap) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "MySQLManager::query, get(" << name
            << ") fail, format=" << format;
        return nullptr;
    }
    return conn->query(format, ap);
}

MySQLRes::ptr MySQLManager::query(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "MySQLManager::query, get(" << name
            << ") fail, sql=" << sql;
        return nullptr;
    }
    return conn->query(sql);
}

MySQLTransaction::ptr MySQLManager::openTransaction(const std::string& name, bool auto_commit) {
    auto conn = get(name);
    if(!conn) {
        SYLAR_LOG_ERROR(g_logger) << "MySQLManager::openTransaction, get(" << name
            << ") fail";
        return nullptr;
    }
    MySQLTransaction::ptr trans(MySQLTransaction::Create(conn, auto_commit));
    return trans;
}

void MySQLManager::freeMySQL(const std::string& name, MySQL* m) {
    if(m->m_mysql) {
        MutexType::Lock lock(m_mutex);
        if(m_conns[name].size() < m_maxConn) {
            m_conns[name].push_back(m);
            return;
        }
    }
    delete m;
}

}
