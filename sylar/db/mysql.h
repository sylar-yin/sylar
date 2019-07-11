#ifndef __SYLAR_DB_MYSQL_H__
#define __SYLAR_DB_MYSQL_H__

#include <mysql/mysql.h>
#include <memory>
#include <functional>
#include <map>
#include "sylar/mutex.h"

namespace sylar {

//typedef std::shared_ptr<MYSQL_RES> MySQLResPtr;
//typedef std::shared_ptr<MYSQL> MySQLPtr;
class MySQL;
class IMySQLUpdate {
public:
    typedef std::shared_ptr<IMySQLUpdate> ptr;
    virtual ~IMySQLUpdate() {}
    virtual int cmd(const char* format, ...) = 0;
    virtual int cmd(const char* format, va_list ap) = 0;
    virtual int cmd(const std::string& sql) = 0;
    virtual std::shared_ptr<MySQL> getMySQL() = 0;
};

class MySQLRes {
public:
    typedef std::shared_ptr<MySQLRes> ptr;
    typedef std::function<bool(MYSQL_ROW row
                ,int field_count, int row_no)> data_cb;
    MySQLRes(MYSQL_RES* res, int eno, const char* estr);

    MYSQL_RES* get() const { return m_data.get();}

    uint64_t getRows();
    uint64_t getFields();

    int getErrno() const { return m_errno;}
    const std::string& getErrStr() const { return m_errstr;}

    bool foreach(data_cb cb);
private:
    int m_errno;
    std::string m_errstr;
    std::shared_ptr<MYSQL_RES> m_data;
};

class MySQLManager;
class MySQL : public IMySQLUpdate {
friend class MySQLManager;
public:
    typedef std::shared_ptr<MySQL> ptr;

    MySQL(const std::map<std::string, std::string>& args);

    bool connect();
    bool ping();

    virtual int cmd(const char* format, ...) override;
    virtual int cmd(const char* format, va_list ap) override;
    virtual int cmd(const std::string& sql) override;
    virtual std::shared_ptr<MySQL> getMySQL() override;

    uint64_t getAffectedRows();

    MySQLRes::ptr query(const char* format, ...);
    MySQLRes::ptr query(const char* format, va_list ap); 
    MySQLRes::ptr query(const std::string& sql);

    const char* cmd();

    bool use(const std::string& dbname);
    const char* getError();
    uint64_t getInsertId();
private:
    bool isNeedCheck();
private:
    std::map<std::string, std::string> m_params;
    std::shared_ptr<MYSQL> m_mysql;

    std::string m_cmd;
    std::string m_dbname;

    uint64_t m_lastUsedTime;
    bool m_hasError;
};

class MySQLTransaction : public IMySQLUpdate {
public:
    typedef std::shared_ptr<MySQLTransaction> ptr;

    static MySQLTransaction::ptr Create(MySQL::ptr mysql, bool auto_commit);
    ~MySQLTransaction();

    bool commit();
    bool rollback();

    virtual int cmd(const char* format, ...) override;
    virtual int cmd(const char* format, va_list ap) override;
    virtual int cmd(const std::string& sql) override;
    virtual std::shared_ptr<MySQL> getMySQL() override;

    bool isAutoCommit() const { return m_autoCommit;}
    bool isFinished() const { return m_isFinished;}
    bool isError() const { return m_hasError;}
private:
    MySQLTransaction(MySQL::ptr mysql, bool auto_commit);
private:
    MySQL::ptr m_mysql;
    bool m_autoCommit;
    bool m_isFinished;
    bool m_hasError;
};

class MySQLManager {
public:
    typedef sylar::Mutex MutexType;

    MySQLManager();
    ~MySQLManager();

    MySQL::ptr get(const std::string& name);
    void registerMySQL(const std::string& name, const std::map<std::string, std::string>& params);

    void checkConnection(int sec = 30);

    uint32_t getMaxConn() const { return m_maxConn;}
    void setMaxConn(uint32_t v) { m_maxConn = v;}

    int cmd(const std::string& name, const char* format, ...);
    int cmd(const std::string& name, const char* format, va_list ap);
    int cmd(const std::string& name, const std::string& sql);

    MySQLRes::ptr query(const std::string& name, const char* format, ...);
    MySQLRes::ptr query(const std::string& name, const char* format, va_list ap); 
    MySQLRes::ptr query(const std::string& name, const std::string& sql);

    MySQLTransaction::ptr openTransaction(const std::string& name, bool auto_commit);
private:
    void freeMySQL(const std::string& name, MySQL* m);
private:
    uint32_t m_maxConn;
    MutexType m_mutex;
    std::map<std::string, std::list<MySQL*> > m_conns;
    std::map<std::string, std::map<std::string, std::string> > m_dbDefines;
};


}

#endif
