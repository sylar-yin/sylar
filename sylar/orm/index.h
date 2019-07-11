#ifndef __SYLAR_ORM_INDEX_H__
#define __SYLAR_ORM_INDEX_H__

#include <memory>
#include <string>
#include <vector>
#include "tinyxml2.h"

namespace sylar {
namespace orm {

class Index {
public:
    enum Type {
        TYPE_NULL = 0,
        TYPE_PK,
        TYPE_UNIQ,
        TYPE_INDEX
    };
    typedef std::shared_ptr<Index> ptr;
    const std::string& getName() const { return m_name;}
    const std::string& getType() const { return m_type;}
    const std::string& getDesc() const { return m_desc;}
    const std::vector<std::string>& getCols() const { return m_cols;}
    Type getDType() const { return m_dtype;}
    bool init(const tinyxml2::XMLElement& node);

    bool isPK() const { return m_type == "pk";}

    static Type ParseType(const std::string& v);
    static std::string TypeToString(Type v);
private:
    std::string m_name;
    std::string m_type;
    std::string m_desc;
    std::vector<std::string> m_cols;

    Type m_dtype;
};

}
}

#endif
