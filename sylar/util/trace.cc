#include "trace.h"
#include "sylar/util.h"

namespace sylar {

TimeCalc::TimeCalc()
    :m_time(sylar::GetCurrentUS()) {
}

uint64_t TimeCalc::elapse() const {
    return sylar::GetCurrentUS() - m_time;
}

void TimeCalc::tick(const std::string& name) {
    m_timeLine.push_back(std::make_pair(name, elapse()));
}

std::string TimeCalc::toString() const {
    std::stringstream ss;
    uint64_t last = 0;
    for(size_t i = 0; i < m_timeLine.size(); ++i) {
        ss << "(" << m_timeLine[i].first << ":" << (m_timeLine[i].second - last) << ")";
        last = m_timeLine[i].second;
    }
    return ss.str();
}

}
