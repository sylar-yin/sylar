#ifndef __SYLAR_DB_DB_H__
#define __SYLAR_DB_DB_H__

#include <memory>
#include <string>

namespace sylar {

class ISQLData {
public:
    typedef std::shared_ptr<ISQLData> ptr;
    virtual ~ISQLData() {}

    virtual int getErrno() const = 0;
    virtual const std::string& getErrStr() const = 0;

    virtual int getDataCount() = 0;
    virtual int getColumnCount() = 0;
    virtual int getColumnBytes(int idx) = 0;
    virtual int getColumnType(int idx) = 0;
    virtual std::string getColumnName(int idx) = 0;

    virtual bool isNull(int idx) = 0;
    virtual int8_t getInt8(int idx) = 0;
    virtual uint8_t getUint8(int idx) = 0;
    virtual int16_t getInt16(int idx) = 0;
    virtual uint16_t getUint16(int idx) = 0;
    virtual int32_t getInt32(int idx) = 0;
    virtual uint32_t getUint32(int idx) = 0;
    virtual int64_t getInt64(int idx) = 0;
    virtual uint64_t getUint64(int idx) = 0;
    virtual float getFloat(int idx) = 0;
    virtual double getDouble(int idx) = 0;
    virtual std::string getString(int idx) = 0;
    virtual std::string getBlob(int idx) = 0;
    virtual time_t getTime(int idx) = 0;
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

class IStmt {
public:
    typedef std::shared_ptr<IStmt> ptr;
    virtual ~IStmt(){}
    virtual int bindInt8(int idx, int8_t& value) = 0;
    virtual int bindUint8(int idx, uint8_t& value) = 0;
    virtual int bindInt16(int idx, int16_t& value) = 0;
    virtual int bindUint16(int idx, uint16_t& value) = 0;
    virtual int bindInt32(int idx, int32_t& value) = 0;
    virtual int bindUint32(int idx, uint32_t& value) = 0;
    virtual int bindInt64(int idx, int64_t& value) = 0;
    virtual int bindUint64(int idx, uint64_t& value) = 0;
    virtual int bindFloat(int idx, float& value) = 0;
    virtual int bindDouble(int idx, double& value) = 0;
    virtual int bindString(int idx, char* value) = 0;
    virtual int bindString(int idx, std::string& value) = 0;
    virtual int bindBlob(int idx, void* value, int64_t size) = 0;
    virtual int bindBlob(int idx, std::string& value) = 0;
    virtual int bindTime(int idx, time_t value) = 0;
    virtual int bindNull(int idx) = 0;

    virtual int execute() = 0;
    virtual int64_t getLastInsertId() = 0;
    virtual ISQLData::ptr query() = 0;

    virtual int getErrno() = 0;
    virtual std::string getErrStr() = 0;
};

class IDB : public ISQLUpdate
            ,public ISQLQuery {
public:
    typedef std::shared_ptr<IDB> ptr;
    virtual ~IDB() {}

    virtual IStmt::ptr prepare(const std::string& stmt) = 0;
    virtual int getErrno() = 0;
    virtual std::string getErrStr() = 0;
};

}

#endif
