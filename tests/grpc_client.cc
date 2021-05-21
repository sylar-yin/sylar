#include "sylar/grpc/grpc_stream.h"
#include "tests/test.pb.h"

void run() {
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("127.0.0.1:8099");
    sylar::grpc::GrpcConnection::ptr conn(new sylar::grpc::GrpcConnection);

    if(!conn->connect(addr, false)) {
        std::cout << "connect " << *addr << " fail" << std::endl;
        return;
    }
    conn->start();

    std::string  prefx;
    for(int i = 0; i < 1000; ++i) {
        //prefx += "a";
    }

    for(int x = 0; x < 2; ++x) {
        sylar::IOManager::GetThis()->schedule([conn, x, prefx](){
            int fail = 0;
            int error = 0;
            static int sn = 0;
            for(int i = 0; i < 10; ++i) {
                test::HelloRequest hr;
                auto tmp = sylar::Atomic::addFetch(sn);
                hr.set_id(prefx + "hello_" + std::to_string(tmp));
                hr.set_msg(prefx + "world_" + std::to_string(tmp));
                auto rsp = conn->request("/test.HelloService/Hello", hr, 10000);
                std::cout << rsp->toString() << std::endl;
                if(rsp->getResponse()) {
                    //std::cout << *rsp->getResponse() << std::endl;

                    auto hrp = rsp->getAsPB<test::HelloResponse>();
                    if(hrp) {
                        //std::cout << sylar::PBToJsonString(*hrp) << std::endl;
                        std::cout << "========" << rsp->getResult() << std::endl;
                    } else {
                        std::cout << "########" << rsp->getResult() << std::endl;
                        ++error;
                    }
                } else {
                    ++fail;
                }
            }
            std::cout << "=======fail=" << fail << " error=" << error << " =========" << std::endl;
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
