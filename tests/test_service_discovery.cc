#include "sylar/streams/service_discovery.h"
#include "sylar/iomanager.h"
#include "sylar/rock/rock_stream.h"
#include "sylar/log.h"
#include "sylar/worker.h"

sylar::ZKServiceDiscovery::ptr zksd(new sylar::ZKServiceDiscovery("127.0.0.1:21812"));
sylar::RockSDLoadBalance::ptr rsdlb(new sylar::RockSDLoadBalance(zksd));

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

std::atomic<uint32_t> s_id;
void on_timer() {
    g_logger->setLevel(sylar::LogLevel::INFO);
    //SYLAR_LOG_INFO(g_logger) << "on_timer";
    sylar::RockRequest::ptr req(new sylar::RockRequest);
    req->setSn(++s_id);
    req->setCmd(100);
    req->setBody("hello");

    auto rt = rsdlb->request("sylar.top", "blog", req, 1000);
    if(!rt->response) {
        if(req->getSn() % 50 == 0) {
            SYLAR_LOG_ERROR(g_logger) << "invalid response: " << rt->toString();
        }
    } else {
        if(req->getSn() % 1000 == 0) {
            SYLAR_LOG_INFO(g_logger) << rt->toString();
        }
    }
}

void run() {
    zksd->setSelfInfo("127.0.0.1:2222");
    zksd->setSelfData("aaaa");

    std::unordered_map<std::string, std::unordered_map<std::string,std::string> > confs;
    confs["sylar.top"]["blog"] = "fair";
    rsdlb->start(confs);
    //SYLAR_LOG_INFO(g_logger) << "on_timer---";

    sylar::IOManager::GetThis()->addTimer(1, on_timer, true);
}

int main(int argc, char** argv) {
    sylar::WorkerMgr::GetInstance()->init({
        {"service_io", {
            {"thread_num", "1"}
        }}
    });
    sylar::IOManager iom(1);
    iom.addTimer(1000, [](){
            std::cout << rsdlb->statusString() << std::endl;
    }, true);
    iom.schedule(run);
    return 0;
}
