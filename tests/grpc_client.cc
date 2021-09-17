#include "sylar/grpc/grpc_stream.h"
#include "tests/test.pb.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void run() {
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("10.104.16.110:8099");
    sylar::grpc::GrpcConnection::ptr conn(new sylar::grpc::GrpcConnection);

    if(!conn->connect(addr, false)) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " fail" << std::endl;
        return;
    }
    conn->start();

    std::string  prefx = "aaaa";
    for(int i = 0; i < 4; ++i) {
        prefx = prefx + prefx;
    }

    for(int x = 0; x < 1; ++x) {
        sylar::IOManager::GetThis()->schedule([conn, x, prefx](){
            int fail = 0;
            int error = 0;
            static int sn = 0;
            for(int i = 0; i < 128; ++i) {
                test::HelloRequest hr;
                auto tmp = sylar::Atomic::addFetch(sn);
                hr.set_id(prefx + "hello_" + std::to_string(tmp));
                hr.set_msg(prefx + "world_" + std::to_string(tmp));
                auto rsp = conn->request("/test.HelloService/Hello", hr, 100000);
                SYLAR_LOG_INFO(g_logger) << rsp->toString() << std::endl;
                if(rsp->getResponse()) {
                    //std::cout << *rsp->getResponse() << std::endl;

                    auto hrp = rsp->getAsPB<test::HelloResponse>();
                    if(hrp) {
                        //std::cout << sylar::PBToJsonString(*hrp) << std::endl;
                        SYLAR_LOG_INFO(g_logger) << "========" << rsp->getResult() << " - " << sylar::PBToJsonString(hr);
                    } else { 
                        SYLAR_LOG_INFO(g_logger) << "########" << rsp->getResult() << " - " << sylar::PBToJsonString(hr);
                        ++error;
                    }
                    //SYLAR_LOG_INFO(g_logger) << *rsp->getResponse();
                } else {
                    ++fail;
                }
            }
            SYLAR_LOG_INFO(g_logger) << "=======fail=" << fail << " error=" << error << " =========" << std::endl;
        });
    }
}

int main(int argc, char** argv) {
    sylar::IOManager io(2);
    io.schedule(run);
    io.addTimer(10000, [](){}, true);
    io.stop();
    return 0;
}
