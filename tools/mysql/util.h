#ifndef __TOOLS_MYSQL_UTIL_H__
#define __TOOLS_MYSQL_UTIL_H__

#include "sylar/sylar.h"
#include "schema.pb.h"

namespace utils {

typedef std::shared_ptr<sylar::schema::Table> TablePtr;

TablePtr InitTableInfo(const std::string& dbname,  const std::string& db, const std::string& table);

}

#endif
