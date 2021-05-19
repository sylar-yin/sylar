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

    sleep(1);
    test::HelloRequest hr;
    hr.set_id("hello");
    hr.set_msg("world");
    auto rsp = conn->request("/test.HelloService/Hello", hr, 100);
    std::cout << rsp->toString() << std::endl;
}

int main(int argc, char** argv) {
    sylar::IOManager io(2);
    io.schedule(run);
    io.addTimer(10000, [](){}, true);
    io.stop();
    return 0;
}
