#include "index.h"
#include "sylar/log.h"
#include "sylar/util.h"

namespace sylar {
namespace orm {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("orm");

Index::Type Index::ParseType(const std::string& v) {
#define XX(a, b) \
    if(v == b) {\
        return a; \
    }
    XX(TYPE_PK, "pk");
    XX(TYPE_UNIQ, "uniq");
    XX(TYPE_INDEX, "index");
#undef XX
    return TYPE_NULL;
}

std::string Index::TypeToString(Type v) {
#define XX(a, b) \
    if(v == a) { \
        return b; \
    }
    XX(TYPE_PK, "pk");
    XX(TYPE_UNIQ, "uniq");
    XX(TYPE_INDEX, "index");
#undef XX
    return "";
}

bool Index::init(const tinyxml2::XMLElement& node) {
    if(!node.Attribute("name")) {
        SYLAR_LOG_ERROR(g_logger) << "index name not exists";
        return false;
    }
    m_name = node.Attribute("name");

    if(!node.Attribute("type")) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " type is null";
        return false;
    }

    m_type = node.Attribute("type");
    m_dtype = ParseType(m_type);
    if(m_dtype == TYPE_NULL) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " type=" << m_type
            << " invalid (pk, index, uniq)";
        return false;
    }

    if(!node.Attribute("cols")) {
        SYLAR_LOG_ERROR(g_logger) << "index name=" << m_name << " cols is null";
    }
    std::string tmp = node.Attribute("cols");
    m_cols = sylar::split(tmp, ',');

    if(node.Attribute("desc")) {
        m_desc = node.Attribute("desc");
    }
    return true;
}

}
}
