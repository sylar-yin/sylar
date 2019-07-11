#include "util.h"
#include "sylar/util.h"

namespace sylar {
namespace orm {

std::string GetAsVariable(const std::string& v) {
    return sylar::ToLower(v);
}

std::string GetAsClassName(const std::string& v) {
    auto vs = sylar::split(v, '_');
    std::stringstream ss;
    for(auto& i : vs) {
        i[0] = toupper(i[0]);
        ss << i;
    }
    return ss.str();
}

std::string GetAsMemberName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    class_name[0] = tolower(class_name[0]);
    return "m_" + class_name;
}

std::string GetAsGetFunName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    return "get" + class_name;
}

std::string GetAsSetFunName(const std::string& v) {
    auto class_name = GetAsClassName(v);
    return "set" + class_name;
}

std::string XmlToString(const tinyxml2::XMLNode& node) {
    return "";
}

std::string GetAsDefineMacro(const std::string& v) {
    std::string tmp = sylar::replace(v, '.', '_');
    tmp = sylar::ToUpper(tmp);
    return "__" + tmp + "__";
}

}
}
