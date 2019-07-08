#include "sylar/sylar.h"
#include "sylar/rock/rock_stream.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

sylar::RockConnection::ptr conn(new sylar::RockConnection);
void run() {
    conn->setAutoConnect(true);
    sylar::Address::ptr addr = sylar::Address::LookupAny("127.0.0.1:8061");
    if(!conn->connect(addr)) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " false";
    }
    conn->start();

    sylar::IOManager::GetThis()->addTimer(1000, [](){
        sylar::RockRequest::ptr req(new sylar::RockRequest);
        static uint32_t s_sn = 0;
        req->setSn(++s_sn);
        req->setCmd(100);
        req->setBody("hello world sn=" + std::to_string(s_sn));

        auto rsp = conn->request(req, 300);
        if(rsp->response) {
            SYLAR_LOG_INFO(g_logger) << rsp->response->toString();
        } else {
            SYLAR_LOG_INFO(g_logger) << "error result=" << rsp->result;
        }
    }, true);
}

int main(int argc, char** argv) {
    sylar::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
