#include "sylar/db/sqlite3.h"
#include "sylar/log.h"
#include "sylar/util.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_batch(sylar::SQLite3::ptr db) {
    auto ts = sylar::GetCurrentMS();
    int n = 1000000;
    sylar::SQLite3Transaction trans(db);
    //db->execute("PRAGMA synchronous = OFF;");
    trans.begin();
    sylar::SQLite3Stmt::ptr stmt = sylar::SQLite3Stmt::Create(db,
                "insert into user(name, age) values(?, ?)");
    for(int i = 0; i < n; ++i) {
        stmt->reset();
        stmt->bind(1, "batch_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->step();
    }
    trans.commit();
    auto ts2 = sylar::GetCurrentMS();

    SYLAR_LOG_INFO(g_logger) << "used: " << (ts2 - ts) / 1000.0 << "s batch insert n=" << n;
}

int main(int argc, char** argv) {
    const std::string dbname = "test.db";
    auto db = sylar::SQLite3::Create(dbname, sylar::SQLite3::READWRITE);
    if(!db) {
        SYLAR_LOG_INFO(g_logger) << "dbname=" << dbname << " not exists";
        db = sylar::SQLite3::Create(dbname
                , sylar::SQLite3::READWRITE | sylar::SQLite3::CREATE);
        if(!db) {
            SYLAR_LOG_INFO(g_logger) << "dbname=" << dbname << " create error";
            return 0;
        }

#define XX(...) #__VA_ARGS__
        int rt = db->execute(
XX(create table user (
        id integer primary key autoincrement,
        name varchar(50) not null default "",
        age int not null default 0
        )));
#undef XX

        if(rt != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "create table error "
                << db->getErrorCode() << " - " << db->getErrorMsg();
            return 0;
        }
    }

    for(int i = 0; i < 10; ++i) {
        if(db->execute("insert into user(name, age) values(\"name_%d\",%d)", i, i)
                != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "insert into error " << i << " "
                << db->getErrorCode() << " - " << db->getErrorMsg();
        }
    }

    sylar::SQLite3Stmt::ptr stmt = sylar::SQLite3Stmt::Create(db,
                "insert into user(name, age) values(?, ?)");
    if(!stmt) {
        SYLAR_LOG_ERROR(g_logger) << "create statement error "
            << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }

    for(int i = 0; i < 10; ++i) {
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);
        //stmt->bind(3, "stmt_" + std::to_string(i + 1));
        //stmt->bind(4, i + 1);

        if(stmt->execute() != SQLITE_OK) {
            SYLAR_LOG_ERROR(g_logger) << "execute statment error " << i << " "
                << db->getErrorCode() << " - " << db->getErrorMsg();
        }
        stmt->reset();
    }

    sylar::SQLite3Stmt::ptr query = sylar::SQLite3Stmt::Create(db,
            "select * from user");
    if(!query) {
        SYLAR_LOG_ERROR(g_logger) << "create statement error "
            << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }
    auto ds = query->query();
    if(!ds) {
        SYLAR_LOG_ERROR(g_logger) << "query error "
            << db->getErrorCode() << " - " << db->getErrorMsg();
        return 0;
    }

    do {
        SYLAR_LOG_INFO(g_logger) << "query ";
    } while(ds->next());

    test_batch(db);
    return 0;
}
