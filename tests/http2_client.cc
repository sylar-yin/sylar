#include "sylar/sylar.h"
#include "sylar/http2/http2_stream.h"

void test() {
    //sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("192.168.1.6:8099");
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("127.0.0.1:8099");
    sylar::http2::Http2Connection::ptr stream(new sylar::http2::Http2Connection());
    //if(!stream->connect(addr, true)) {
    if(!stream->connect(addr, false)) {
        std::cout << "connect " << *addr << " fail";
    }
    //if(stream->handleShakeClient()) {
    //    std::cout << "handleShakeClient ok," << stream->isConnected() << std::endl;
    //} else {
    //    std::cout << "handleShakeClient fail" << std::endl;
    //    return;
    //}
    stream->start();
    sleep(1);

    for(int x = 0; x < 1; ++x) {
        sylar::IOManager::GetThis()->schedule([stream, x](){
            for(int i = 0; i < 100; ++i) {
                sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
                //req->setHeader(":path", "/");
                //req->setHeader(":method", "GET");
                //req->setHeader(":scheme", "http");
                //req->setHeader(":host", "127.0.0.1");

                req->setHeader(":method", "GET");
                req->setHeader(":scheme", "http");
                req->setHeader(":path", "/_/config?abc=111#cde");
                req->setHeader(":authority", "127.0.0.1:8090");
                req->setHeader("content-type", "text/html");
                req->setHeader("user-agent", "grpc-go/1.37.0");
                req->setHeader("hello", "world");
                req->setHeader("id", std::to_string(x) + "_" + std::to_string(i));
                req->setBody("hello test");

                auto rt = stream->request(req, 100000);
                std::cout << "----" << rt->result << std::endl;
                //sleep(1);
            }
        });
    }
}

int main(int argc, char** argv) {
    sylar::IOManager iom;
    iom.schedule(test);
    iom.addTimer(1000, [](){}, true);
    iom.stop();
    return 0;
}
