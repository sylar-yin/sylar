#include "orm_out/test/orm/user_info.h"
#include "sylar/db/sqlite3.h"
#include "sylar/db/mysql.h"

int main(int argc, char** argv) {
    sylar::IDB::ptr db;
    if(argc == 1) {
        db = sylar::SQLite3::Create("abc.db");
        std::cout << "create table: " << test::orm::UserInfoDao::CreateTableSQLite3(db) << std::endl;
    } else {
        std::map<std::string, std::string> params;
        params["host"] = "127.0.0.1";
        params["user"] = "sylar";
        params["passwd"] = "123456";
        params["dbname"] = "sylar";

        sylar::MySQL::ptr m(new sylar::MySQL(params));
        m->connect();
        db = m;
        std::cout << "create table: " << test::orm::UserInfoDao::CreateTableMySQL(db) << std::endl;
    }
    for(int i = 0; i < 10; ++i) {
        test::orm::UserInfo::ptr u(new test::orm::UserInfo);
        u->setName("name_a" + std::to_string(i));
        u->setEmail("mail_a" + std::to_string(i));
        u->setPhone("phone_a" + std::to_string(i));
        u->setStatus(i % 2);

        std::cout << "i= " << i << " - " << test::orm::UserInfoDao::Insert(u, db);
        std::cout << " - " << u->toJsonString()
                  << std::endl;
    }

    std::vector<test::orm::UserInfo::ptr> us;
    std::cout << "query_by_status: " << test::orm::UserInfoDao::QueryByStatus(us, 1, db) << std::endl;
    for(size_t i = 0; i < us.size(); ++i) {
        std::cout << "i=" << i << " - " << us[i]->toJsonString() << std::endl;
        us[i]->setName(us[i]->getName() + "_new");
        test::orm::UserInfoDao::Update(us[i], db);
    }

    std::cout << "delete: " << test::orm::UserInfoDao::DeleteByStatus(1, db) << std::endl;
    us.clear();
    std::cout << "query_by_status: " << test::orm::UserInfoDao::QueryByStatus(us, 0, db) << std::endl;
    for(size_t i = 0; i < us.size(); ++i) {
        std::cout << "i=" << i << " - " << us[i]->toJsonString() << std::endl;
    }

    us.clear();
    std::cout << "query_all: " << test::orm::UserInfoDao::QueryAll(us, db) << std::endl;
    for(size_t i = 0; i < us.size(); ++i) {
        std::cout << "i=" << i << " - " << us[i]->toJsonString() << std::endl;
    }
    return 0;
}
