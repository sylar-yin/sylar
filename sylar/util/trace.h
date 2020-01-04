#ifndef __SYLAR_UTIL_TRACE_H__
#define __SYLAR_UTIL_TRACE_H__

#include <string>
#include <vector>
#include <map>

namespace sylar {

//时间微秒
class TimeCalc {
public:
    TimeCalc();
    uint64_t elapse() const;

    void tick(const std::string& name);
    const std::vector<std::pair<std::string, uint64_t> > getTimeLine() const { return m_timeLine;}

    std::string toString() const;
private:
    uint64_t m_time;
    std::vector<std::pair<std::string, uint64_t> > m_timeLine;
};

}

#endif
