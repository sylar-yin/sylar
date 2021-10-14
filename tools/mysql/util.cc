#include "util.h"

namespace utils {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

bool InitTableColumn(TablePtr tb, const std::string& dbname,  const std::string& db, const std::string& table) {
    std::string sql = "SELECT COLUMN_NAME, COLUMN_DEFAULT, IS_NULLABLE, DATA_TYPE, COLUMN_TYPE, COLUMN_COMMENT"
        " FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = \"%s\" AND TABLE_NAME = \"%s\"";

    auto data = sylar::MySQLUtil::TryQuery(dbname, 2, sql.c_str(), db.c_str(), table.c_str());
    if(!data) {
        SYLAR_LOG_ERROR(g_logger) << "Get ColumnInfo fail, db_name=" << dbname << " db=" << db << " table=" << table;
        return false;
    }

    while(data->next()) {
        auto field = tb->add_fields();
        field->set_name(data->getString("COLUMN_NAME"));
        field->set_type(data->getString("COLUMN_TYPE"));
        field->set_comment(data->getString("COLUMN_COMMENT"));
    }
    return true;
}

TablePtr InitTableInfo(const std::string& dbname,  const std::string& db, const std::string& table) {
    //Table info 
    std::string table_info_sql = "SELECT TABLE_COMMENT"
        " FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = \"%s\" AND TABLE_NAME = \"%s\"";
    auto data = sylar::MySQLUtil::TryQuery(dbname, 2, table_info_sql.c_str(), db.c_str(), table.c_str());
    if(!data) {
        SYLAR_LOG_ERROR(g_logger) << "Get TableInfo fail, db_name=" << dbname << " db=" << db << " table=" << table;
        return nullptr;
    }

    if(!data->next()) {
        SYLAR_LOG_ERROR(g_logger) << "Get TableInfo fail, db_name=" << dbname << " db=" << db <<" table=" << table << " not exits";
        return nullptr;
    }
    TablePtr tb = std::make_shared<sylar::schema::Table>();
    auto comment = data->getString(0);
    tb->set_name(table);
    tb->set_comment(comment);
    
    //Column info
    if(!InitTableColumn(tb, dbname, db, table)) {
        SYLAR_LOG_ERROR(g_logger) << "InitTableColumn fail db_name=" << dbname << " db=" << db << " table=" << table;
    }
    return tb;
}

}
