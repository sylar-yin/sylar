#ifndef __SYLAR_DB_DB_H__
#define __SYLAR_DB_DB_H__

#include <memory>
#include <string>

namespace sylar {

class ISQLData {
public:
    typedef std::shared_ptr<ISQLData> ptr;
    virtual ~ISQLData() {}

    virtual int getDataCount() = 0;
    virtual int getColumnCount() = 0;
    virtual int getColumnBytes(int idx) = 0;
    virtual int getColumnType(int idx) = 0;
    virtual std::string getColumnName(int idx) = 0;

    virtual int getInt(int idx) = 0;
    virtual double getDouble(int idx) = 0;
    virtual int64_t getInt64(int idx) = 0;
    virtual const char* getText(int idx) = 0;
    virtual std::string getTextString(int idx) = 0;
    virtual std::string getBlob(int idx) = 0;
    virtual bool next() = 0;
};

class ISQLUpdate {
public:
    virtual ~ISQLUpdate() {}
    virtual int execute(const char* format, ...) = 0;
    virtual int execute(const std::string& sql) = 0;
    virtual int64_t getLastInsertId() = 0;
};

class ISQLQuery {
public:
    virtual ~ISQLQuery() {}
    virtual ISQLData::ptr query(const char* format, ...) = 0;
    virtual ISQLData::ptr query(const std::string& sql) = 0;
};

}

#endif
