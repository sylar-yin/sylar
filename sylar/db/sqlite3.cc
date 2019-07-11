#include "sqlite3.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

SQLite3::SQLite3(sqlite3* db)
    :m_db(db) {
}

SQLite3::~SQLite3() {
    close();
}

SQLite3::ptr SQLite3::Create(sqlite3* db) {
    SQLite3::ptr rt(new SQLite3(db));
    return rt;
}

SQLite3::ptr SQLite3::Create(const std::string& dbname ,int flags) {
    sqlite3* db;
    if(sqlite3_open_v2(dbname.c_str(), &db, flags, nullptr) == SQLITE_OK) {
        return SQLite3::ptr(new SQLite3(db));
    }
    return nullptr;
}

int SQLite3::getErrorCode() const {
    return sqlite3_errcode(m_db);
}

std::string SQLite3::getErrorMsg() const {
    return sqlite3_errmsg(m_db);
}

int SQLite3::close() {
    int rc = SQLITE_OK;
    if(m_db) {
        rc = sqlite3_close(m_db);
        if(rc == SQLITE_OK) {
            m_db = nullptr;
        }
    }
    return rc;
}

int SQLite3::execute(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
    va_end(ap);
    return sqlite3_exec(m_db, sql.get(), 0, 0, 0);
}

ISQLData::ptr SQLite3::query(const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    std::shared_ptr<char> sql(sqlite3_vmprintf(format, ap), sqlite3_free);
    va_end(ap);
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.get());
    if(!stmt) {
        return nullptr;
    }
    return stmt->query();
}

ISQLData::ptr SQLite3::query(const std::string& sql) {
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.c_str());
    if(!stmt) {
        return nullptr;
    }
    return stmt->query();
}

int SQLite3::execute(const std::string& sql) {
    return sqlite3_exec(m_db, sql.c_str(), 0, 0, 0);
}

int64_t SQLite3::getLastInsertId() {
    return sqlite3_last_insert_rowid(m_db);
}

SQLite3Stmt::ptr SQLite3Stmt::Create(SQLite3::ptr db, const char* stmt) {
    SQLite3Stmt::ptr rt(new SQLite3Stmt(db));
    if(rt->prepare(stmt) != SQLITE_OK) {
        return nullptr;
    }
    return rt;
}

int SQLite3Stmt::bind(int idx, int32_t value) {
    return sqlite3_bind_int(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, double value) {
    return sqlite3_bind_double(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, int64_t value) {
    return sqlite3_bind_int64(m_stmt, idx, value);
}

int SQLite3Stmt::bind(int idx, const char* value, Type type) {
    return sqlite3_bind_text(m_stmt, idx, value, strlen(value)
            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const void* value, int len, Type type) {
    return sqlite3_bind_blob(m_stmt, idx, value, len
            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const std::string& value, Type type) {
    return sqlite3_bind_text(m_stmt, idx, value.c_str(), value.size()
            ,type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx) {
    return sqlite3_bind_null(m_stmt, idx);
}

int SQLite3Stmt::bind(const char* name, int32_t value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, double value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, int64_t value) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char* name, const char* value, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, type);
}

int SQLite3Stmt::bind(const char* name, const void* value, int len, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, len, type);
}

int SQLite3Stmt::bind(const char* name, const std::string& value, Type type) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx, value, type);
}

int SQLite3Stmt::bind(const char* name) {
    auto idx = sqlite3_bind_parameter_index(m_stmt, name);
    return bind(idx);
}

int SQLite3Stmt::step() {
    return sqlite3_step(m_stmt);
}

int SQLite3Stmt::reset() {
    return sqlite3_reset(m_stmt);
}

int SQLite3Stmt::prepare(const char* stmt) {
    auto rt = finish();
    if(rt != SQLITE_OK) {
        return rt;
    }
    return sqlite3_prepare_v2(m_db->getDB(), stmt, strlen(stmt), &m_stmt, nullptr);
}

int SQLite3Stmt::finish() {
    auto rc = SQLITE_OK;
    if(m_stmt) {
        rc = sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
    return rc;
}

SQLite3Stmt::SQLite3Stmt(SQLite3::ptr db)
    :m_db(db)
    ,m_stmt(nullptr) {
}

SQLite3Stmt::~SQLite3Stmt() {
    finish();
}

SQLite3Data::ptr SQLite3Stmt::query() {
    if(step() == SQLITE_ROW) {
        return SQLite3Data::ptr(new SQLite3Data(shared_from_this()));
    }
    return nullptr;
}

int SQLite3Stmt::execute() {
    int rt = step();
    if(rt == SQLITE_DONE) {
        rt = SQLITE_OK;
    }
    return rt;
}

SQLite3Data::SQLite3Data(SQLite3Stmt::ptr stmt)
    :m_stmt(stmt) {
}

int SQLite3Data::getDataCount() {
    return sqlite3_data_count(m_stmt->m_stmt);
}

int SQLite3Data::getColumnCount() {
    return sqlite3_column_count(m_stmt->m_stmt);
}

int SQLite3Data::getColumnBytes(int idx) {
    return sqlite3_column_bytes(m_stmt->m_stmt, idx);
}

int SQLite3Data::getColumnType(int idx) {
    return sqlite3_column_type(m_stmt->m_stmt, idx);
}

std::string SQLite3Data::getColumnName(int idx) {
    const char* name = sqlite3_column_name(m_stmt->m_stmt, idx);
    if(name) {
        return name;
    }
    return "";
}

int SQLite3Data::getInt(int idx) {
    return sqlite3_column_int(m_stmt->m_stmt, idx);
}

double SQLite3Data::getDouble(int idx) {
    return sqlite3_column_double(m_stmt->m_stmt, idx);
}

int64_t SQLite3Data::getInt64(int idx) {
    return sqlite3_column_int64(m_stmt->m_stmt, idx);
}

const char* SQLite3Data::getText(int idx) {
    return (const char*)sqlite3_column_text(m_stmt->m_stmt, idx);
}

std::string SQLite3Data::getTextString(int idx) {
    const char* v = getText(idx);
    if(v) {
        return std::string(v, getColumnBytes(idx));
    }
    return "";
}

std::string SQLite3Data::getBlob(int idx) {
    const char* v = (const char*)sqlite3_column_blob(m_stmt->m_stmt, idx);
    if(v) {
        return std::string(v, getColumnType(idx));
    }
    return "";
}

bool SQLite3Data::next() {
    return m_stmt->step() == SQLITE_ROW;
}

SQLite3Transaction::SQLite3Transaction(SQLite3::ptr db, bool auto_commit, Type type)
    :m_db(db)
    ,m_type(type)
    ,m_status(0)
    ,m_autoCommit(auto_commit) {
}

SQLite3Transaction::~SQLite3Transaction() {
    if(m_status == 1) {
        if(m_autoCommit) {
            commit();
        } else {
            rollback();
        }

        if(m_status == 1) {
            SYLAR_LOG_ERROR(g_logger) << m_db << " auto_commit=" << m_autoCommit << " fail";
        }
    }
}

int SQLite3Transaction::begin() {
    if(m_status == 0) {
        const char* sql = "BEGIN";
        if(m_type == IMMEDIATE) {
            sql = "BEGIN IMMEDIATE";
        } else if(m_type == EXCLUSIVE) {
            sql = "BEGIN EXCLUSIVE";
        }
        int rt = m_db->execute(sql);
        if(rt == SQLITE_OK) {
            m_status = 1;
        }
        return rt;
    }
    return -1;
}

int SQLite3Transaction::commit() {
    if(m_status == 1) {
        int rc = m_db->execute("COMMIT");
        if(rc == SQLITE_OK) {
            m_status = 2;
        }
        return rc;
    }
    return -2;
}

int SQLite3Transaction::rollback() {
    if(m_status == 1) {
        int rc = m_db->execute("ROLLBACK");
        if(rc == SQLITE_OK) {
            m_status = 3;
        }
        return rc;
    }
    return -3;
}

}
