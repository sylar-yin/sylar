#include "sylar/sylar.h"
#include "sylar/db/tair.h"
#include <map>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
sylar::Tair::ptr s_tair(new sylar::Tair);
sylar::Tair::ptr s_tair2(new sylar::Tair);

uint64_t s_ts1 = 0;
uint64_t s_ts2 = 0;

uint64_t s_total = 0;
uint64_t s_total2 = 0;
uint64_t s_total3 = 0;

uint64_t s_error = 0;

std::vector<std::string> s_strings;

void do_insert(uint32_t step, uint32_t batch) {
    uint64_t ts = sylar::GetCurrentMS();
    std::string v;
    for(size_t i = 0; i < 100000000; ++i) {
        int rt = s_tair->put("ytest_" + std::to_string(i * batch + step), s_strings[i % s_strings.size()], 1, 3600 * 24 * 2);
        if(rt) {
            //SYLAR_LOG_ERROR(g_logger) << "error i=" << i << " rt=" << rt
            //        << " fibers=" << sylar::Fiber::TotalFibers();
            sylar::Atomic::addFetch(s_error, 1);

            //std::string v;
            //if(!s_tair->get("ytest_" + std::to_string(i * batch + step), v, 1, 100)) {
            //    SYLAR_LOG_INFO(g_logger) << v << ", " << (v == s_strings[i % s_strings.size()]);
            //    SYLAR_ASSERT(v == s_strings[i % s_strings.size()]);
            //}

        }
        sylar::Atomic::addFetch(s_total, 1);
        sylar::Atomic::addFetch(s_total3, 1);
    }
    SYLAR_LOG_INFO(g_logger) << "used: " << (sylar::GetCurrentMS() - ts) / 1000.0 << "s"
        << " fibers=" << sylar::Fiber::TotalFibers();

}

void do_some() {
    uint64_t ts = sylar::GetCurrentMS();
    std::string v;
    for(size_t i = 0; i < 1000000000; ++i) {
        int rt = (i%2==0) ? s_tair->get("atest_" + std::to_string(1000000000ul + i), v, 1, 15)
            :s_tair->get("test_" + std::to_string(1000000000ul + i), v, 1, 1000);
        //int rt = s_tair->get("test_" + std::to_string(i), v, 1, 1000);
        //int rt = s_tair->put("test_" + std::to_string(1000000000ul + i), std::to_string(i), 1);
        if(rt) {
            //SYLAR_LOG_ERROR(g_logger) << "error i=" << i << " rt=" << rt
            //        << " fibers=" << sylar::Fiber::TotalFibers();
            sylar::Atomic::addFetch(s_error, 1);
        }
        sylar::Atomic::addFetch(s_total, 1);
        sylar::Atomic::addFetch(s_total3, 1);
    }
    SYLAR_LOG_INFO(g_logger) << "used: " << (sylar::GetCurrentMS() - ts) / 1000.0 << "s"
        << " fibers=" << sylar::Fiber::TotalFibers();
}

void do_get() {
    uint64_t ts = sylar::GetCurrentMS();
    std::string val;
    for(int i = 0; i < 100000000; ++i) {
        int rt = s_tair->get("test_" + std::to_string(i), val, 1, 1000);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "error i=" << i << " rt=" << rt
                    << " fibers=" << sylar::Fiber::TotalFibers();
        } else {
            //std::cout << "i=" << i << " v=" << val << std::endl;
        }

        sylar::Atomic::addFetch(s_total, 1);
    }
    SYLAR_LOG_INFO(g_logger) << "used: " << (sylar::GetCurrentMS() - ts) / 1000.0 << "s"
        << " fibers=" << sylar::Fiber::TotalFibers();
}

void do_get2() {
    uint64_t ts = sylar::GetCurrentMS();
    std::string val;
    for(int i = 0; i < 100000000; ++i) {
        int rt = s_tair2->get("test_" + std::to_string(i), val, 1, 100);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "error i=" << i << " rt=" << rt
                    << " fibers=" << sylar::Fiber::TotalFibers();
        } else {
            //std::cout << "i=" << i << " v=" << val << std::endl;
        }

        sylar::Atomic::addFetch(s_total, 1);
    }
    SYLAR_LOG_INFO(g_logger) << "used: " << (sylar::GetCurrentMS() - ts) / 1000.0 << "s"
        << " fibers=" << sylar::Fiber::TotalFibers();
}

void run() {
    std::map<std::string, std::string> conf = {
        {"master_addr", "192.168.106.217:15198"},
        {"slave_addr", "192.168.106.218:15198"},
        {"group_name", "MT_USER"},
        {"log_level", "error"},
        {"log_file", "./tair.log"},
        {"thread_count", "8"},
        {"timeout", "20000"},
        {"cache.1", "1000,10000"},
        {"cache.2", "1000,10000"}
    };

    s_ts1 = s_ts2 = sylar::GetCurrentMS();

    SYLAR_LOG_INFO(g_logger) << "init: " << s_tair->init(conf);
    SYLAR_LOG_INFO(g_logger) << "startup: " << s_tair->startup();
    SYLAR_LOG_INFO(g_logger) << "info: " << s_tair->toString();
    sleep(1);
    //SYLAR_LOG_INFO(g_logger) << "init: " << s_tair2->init(conf);
    //SYLAR_LOG_INFO(g_logger) << "startup: " << s_tair2->startup();
    for(int i = 0; i < 1024; ++i) {
        s_strings.push_back(sylar::random_string(1024 + i));
    }

    for(int i = 0; i < 400; ++i) {
        //if(i % 2 == 0) {
            sylar::IOManager::GetThis()->schedule(std::bind(do_insert, i, 400));
            //sylar::IOManager::GetThis()->schedule(do_some);
            //sylar::IOManager::GetThis()->schedule(do_get);
        //} else {
        //    sylar::IOManager::GetThis()->schedule(do_get2);
        //}
    }

    sylar::IOManager::GetThis()->addTimer(5000, [](){
            auto ts = s_ts1;
            s_ts2 = sylar::GetCurrentMS();
            auto tmp = s_total;
            SYLAR_LOG_INFO(g_logger) << "rows=" << (tmp - s_total2) << " used=" << (s_ts2 - ts) / 1000.0 << "s "
                    << " qps=" << (tmp - s_total2) / ((s_ts2 - ts) / 1000.0)
                    << " fibers=" << sylar::Fiber::TotalFibers()
                    << " s_error=" << s_error
                    << " s_total3=" << s_total3
                    << " err_rate=" << (s_error * 100.0 / s_total3);

            s_ts1 = s_ts2;
            s_ts2 = ts;
            s_total2 = tmp;
    }, true);
}

int main(int argc, char** argv) {
    srand(time(0));
    g_logger->setLevel(sylar::LogLevel::INFO);
    sylar::IOManager iom(12);
    iom.schedule(run);
    iom.addTimer(1000, [](){}, true);
    return 0;
}
