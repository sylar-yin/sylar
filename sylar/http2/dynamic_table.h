#ifndef __SYLAR_HTTP2_DYNAMIC_TABLE_H__
#define __SYLAR_HTTP2_DYNAMIC_TABLE_H__

#include <vector>
#include <string>

namespace sylar {
namespace http2 {

class DynamicTable {
public:
    DynamicTable();
    int32_t update(const std::string& name, const std::string& value);
    int32_t findIndex(const std::string& name) const;
    std::pair<int32_t, bool> findPair(const std::string& name, const std::string& value) const;
    std::pair<std::string, std::string> getPair(uint32_t idx) const;
    std::string getName(uint32_t idx) const;
    std::string toString() const;

public:
    static std::pair<std::string, std::string> GetStaticHeaders(uint32_t idx);
    static int32_t GetStaticHeadersIndex(const std::string& name);
    static std::pair<int32_t, bool> GetStaticHeadersPair(const std::string& name, const std::string& val);
private:
    int32_t m_maxDataSize;
    int32_t m_dataSize;
    std::vector<std::pair<std::string, std::string> > m_datas;
};

}
}

#endif
