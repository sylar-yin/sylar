/**
 * @file sqlite3.h
 * @brief SQLite3封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-07-07
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_DB_SQLITE3_H__
#define __SYLAR_DB_SQLITE3_H__

#include <sqlite3.h>
#include <memory>
#include <string>
#include "sylar/noncopyable.h"
#include "db.h"

namespace sylar {

class SQLite3Stmt;

namespace {
template<size_t N, typename... Args>
struct SQLite3Binder {
    static int Bind(std::shared_ptr<SQLite3Stmt> stmt) { return SQLITE_OK; }
};
}

class SQLite3 : public ISQLUpdate, public ISQLQuery
              , public std::enable_shared_from_this<SQLite3> {
public:
    enum Flags {
        READONLY = SQLITE_OPEN_READONLY,
        READWRITE = SQLITE_OPEN_READWRITE,
        CREATE = SQLITE_OPEN_CREATE
    };
    typedef std::shared_ptr<SQLite3> ptr;
    static SQLite3::ptr Create(sqlite3* db);
    static SQLite3::ptr Create(const std::string& dbname
            ,int flags = READWRITE | CREATE);
    ~SQLite3();

    int getErrorCode() const;
    std::string getErrorMsg() const;

    int execute(const char* format, ...) override;
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query(const char* format, ...) override;
    ISQLData::ptr query(const std::string& sql) override;

    template<typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    template<class... Args>
    ISQLData::ptr queryStmt(const char* stmt, const Args&... args);

    int close();

    sqlite3* getDB() const { return m_db;}
private:
    SQLite3(sqlite3* db);
private:
    sqlite3* m_db;
};

class SQLite3Stmt;
class SQLite3Data : public ISQLData {
public:
    typedef std::shared_ptr<SQLite3Data> ptr;
    SQLite3Data(std::shared_ptr<SQLite3Stmt> stmt);

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx);
    int getColumnType(int idx);

    std::string getColumnName(int idx);

    int getInt(int idx);
    double getDouble(int idx);
    int64_t getInt64(int idx);
    const char* getText(int idx);
    std::string getTextString(int idx);
    std::string getBlob(int idx);

    bool next();
private:
    std::shared_ptr<SQLite3Stmt> m_stmt;
};

class SQLite3Stmt : public std::enable_shared_from_this<SQLite3Stmt> {
friend class SQLite3Data;
public:
    typedef std::shared_ptr<SQLite3Stmt> ptr;
    enum Type {
        COPY = 1,
        REF = 2
    };
    static SQLite3Stmt::ptr Create(SQLite3::ptr db, const char* stmt);

    virtual ~SQLite3Stmt();
    int prepare(const char* stmt);
    int finish();

    int bind(int idx, int32_t value);
    int bind(int idx, double value);
    int bind(int idx, int64_t value);
    int bind(int idx, const char* value, Type type = COPY);
    int bind(int idx, const void* value, int len, Type type = COPY);
    int bind(int idx, const std::string& value, Type type = COPY);
    // for null type
    int bind(int idx);

    int bind(const char* name, int32_t value);
    int bind(const char* name, double value);
    int bind(const char* name, int64_t value);
    int bind(const char* name, const char* value, Type type = COPY);
    int bind(const char* name, const void* value, int len, Type type = COPY);
    int bind(const char* name, const std::string& value, Type type = COPY);
    // for null type
    int bind(const char* name);

    int step();
    int reset();

    SQLite3Data::ptr query();
    int execute();
protected:
    SQLite3Stmt(SQLite3::ptr db);
protected:
    SQLite3::ptr m_db;
    sqlite3_stmt* m_stmt;
};

class SQLite3Transaction : Noncopyable {
public:
    enum Type {
        DEFERRED = 0,
        IMMEDIATE = 1,
        EXCLUSIVE = 2
    };
    SQLite3Transaction(SQLite3::ptr db
                       ,bool auto_commit = false 
                       ,Type type = DEFERRED);
    ~SQLite3Transaction();
    int begin();
    int commit();
    int rollback();
private:
    SQLite3::ptr m_db;
    Type m_type;
    int8_t m_status;
    bool m_autoCommit;
};

namespace {
template<typename... Args>
int bindX(SQLite3Stmt::ptr stmt, const Args&... args) {
    return SQLite3Binder<1, Args...>::Bind(stmt, args...);
}
}

template<typename... Args>
int SQLite3::execStmt(const char* stmt, Args&&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if(!st) {
        return -1;
    }
    int rt = bindX(st, args...);
    if(rt != SQLITE_OK) {
        return rt;
    }
    return st->execute();
}

template<class... Args>
ISQLData::ptr SQLite3::queryStmt(const char* stmt, const Args&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if(!st) {
        return nullptr;
    }
    int rt = bindX(st, args...);
    if(rt != SQLITE_OK) {
        return nullptr;
    }
    return st->query();
}

namespace {

template<size_t N, typename Head, typename... Tail>
struct SQLite3Binder<N, Head, Tail...> {
    static int Bind(SQLite3Stmt::ptr stmt
                    ,const Head&, const Tail&...) {
        //static_assert(false, "invalid type");
        static_assert(sizeof...(Tail) < 0, "invalid type");
        return SQLITE_OK;
    }
};

#define XX(type, type2) \
template<size_t N, typename... Tail> \
struct SQLite3Binder<N, type, Tail...> { \
    static int Bind(SQLite3Stmt::ptr stmt \
                    , const type2& value \
                    , const Tail&... tail) { \
        int rt = stmt->bind(N, value); \
        if(rt != SQLITE_OK) { \
            return rt; \
        } \
        return SQLite3Binder<N + 1, Tail...>::Bind(stmt, tail...); \
    } \
};

template<size_t N, typename... Tail>
struct SQLite3Binder<N, const char(&)[], Tail...> {
    static int Bind(SQLite3Stmt::ptr stmt
                    , const char value[]
                    , const Tail&... tail) {
        int rt = stmt->bind(N, (const char*)value);
        if(rt != SQLITE_OK) {
            return rt;
        }
        return SQLite3Binder<N + 1, Tail...>::Bind(stmt, tail...);
    }
};

XX(char*, char* const);
XX(const char*, char* const);
XX(std::string, std::string);
XX(int8_t, int32_t);
XX(uint8_t, int32_t);
XX(int16_t, int32_t);
XX(uint16_t, int32_t);
XX(int32_t, int32_t);
XX(uint32_t, int32_t);
XX(int64_t, int64_t);
XX(uint64_t, int64_t);
XX(float, double);
XX(double, double);
#undef XX
}

}

#endif
