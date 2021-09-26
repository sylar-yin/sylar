#include "sylar/module.h"
#include "sylar/singleton.h"
#include <iostream>
#include "sylar/log.h"
#include "sylar/db/redis.h"
#include "sylar/grpc/grpc_stream.h"
#include "sylar/grpc/grpc_servlet.h"
#include "sylar/application.h"
#include "sylar/sylar.h"
#include "tests/test.pb.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class A {
public:
    A() {
        std::cout << "A::A " << this << std::endl;
    }

    ~A() {
        std::cout << "A::~A " << this << std::endl;
    }

};


int32_t HandleTest(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "request *** " << *request;
    response->setBody("hello test");
    response->setHeader("random", std::to_string(time(0)));
    return 0;
}

int32_t HandleHelloServiceHello(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::SocketStream::ptr session) {
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto req = request->getAsPB<test::HelloRequest>();
    if(!req) {
        response->setResult(100);
        response->setError("invalid pb");
        return -1;
    }
    SYLAR_LOG_INFO(g_logger) << "---" << sylar::PBToJsonString(*req) << " - " << req;

    test::HelloResponse rsp;
    //rsp.set_id("hello");
    //rsp.set_msg("world");
    response->setAsPB(rsp);
    return 0;
}

int32_t HandleHelloServiceHelloStreamA(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::SocketStream::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto req = request->getAsPB<test::HelloRequest>();
    if(!req) {
        response->setResult(100);
        response->setError("invalid pb");
        return -1;
    }
    SYLAR_LOG_INFO(g_logger) << "---" << sylar::PBToJsonString(*req) << " - " << req;

    auto stream_id = request->getRequest()->getStreamId();
    auto h2session = std::dynamic_pointer_cast<sylar::http2::Http2Session>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStreamClient::ptr cli = std::make_shared<sylar::grpc::GrpcStreamClient>(sylar::http2::StreamClient::Create(stream));

    while(true) {
        auto rsp = cli->recvMessage<test::HelloResponse>();
        if(rsp) {
            SYLAR_LOG_INFO(g_logger) << "recv " << sylar::PBToJsonString(*rsp);
        } else {
            break;
        }
    }

    test::HelloResponse rsp;
    rsp.set_id("hello");
    rsp.set_msg("world");

    response->setAsPB(rsp);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamA over";
    //for(int i = 0; i < 10; ++i) {
    //    test::HelloResponse rsp;
    //    rsp.set_id("hello");
    //    rsp.set_msg("world");
    //    cli->sendMessage(rsp);
    //    sleep(1);
    //}
    //response->getResponse()->setHeader("content-type", "application/grpc+proto");
    //stream->sendResponse(response->getResponse(), false);
    //stream->sendHeaders({
    //    {"grpc-status", "0"},
    //    {"grpc-message", "test message"},
    //}, true);
    //sleep(100);
    return 0;
}

int32_t HandleHelloServiceHelloStreamB(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::SocketStream::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto req = request->getAsPB<test::HelloRequest>();
    if(!req) {
        response->setResult(100);
        response->setError("invalid pb");
        return -1;
    }
    SYLAR_LOG_INFO(g_logger) << "---" << sylar::PBToJsonString(*req) << " - " << req;

    auto stream_id = request->getRequest()->getStreamId();
    auto h2session = std::dynamic_pointer_cast<sylar::http2::Http2Session>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStreamClient::ptr cli = std::make_shared<sylar::grpc::GrpcStreamClient>(sylar::http2::StreamClient::Create(stream));

    stream->sendResponse(response->getResponse(), false);

    for(int i = 0; i < 10; ++i) {
        test::HelloResponse rsp;
        rsp.set_id("hello");
        rsp.set_msg("world");
        cli->sendMessage(rsp);
        sleep(1);
    }
    stream->sendHeaders({
        {"grpc-status", "0"},
        {"grpc-message", "test message"},
    }, true);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB over";
    return 0;
}

int32_t HandleHelloServiceHelloStreamC(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::SocketStream::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto req = request->getAsPB<test::HelloRequest>();
    if(!req) {
        response->setResult(100);
        response->setError("invalid pb");
        return -1;
    }
    SYLAR_LOG_INFO(g_logger) << "---" << sylar::PBToJsonString(*req) << " - " << req;

    auto stream_id = request->getRequest()->getStreamId();
    auto h2session = std::dynamic_pointer_cast<sylar::http2::Http2Session>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStreamClient::ptr cli = std::make_shared<sylar::grpc::GrpcStreamClient>(sylar::http2::StreamClient::Create(stream));

    stream->sendResponse(response->getResponse(), false);

    auto wg = sylar::WorkerGroup::Create(1);
    wg->schedule([cli](){
        for(int i = 0; i < 5; ++i) {
            test::HelloResponse rsp;
            rsp.set_id("hello");
            rsp.set_msg("world");
            cli->sendMessage(rsp);
            sleep(1);
        }
    });
    //while(true) {
    for(int i = 0; i < 5; ++i) {
        auto rsp = cli->recvMessage<test::HelloResponse>();
        if(!rsp) {
            break;
        }
        SYLAR_LOG_INFO(g_logger) << "recv " << sylar::PBToJsonString(*rsp);
    }
    wg->waitAll();
    stream->sendHeaders({
        {"grpc-status", "0"},
        {"grpc-message", "test message"},
    }, true);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC over";
    return 0;
}

int32_t HandleHelloServiceHelloStreamA2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::grpc::GrpcStreamClient::ptr cli,
                                sylar::http2::Http2Session::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    while(true) {
        auto rsp = cli->recvMessage<test::HelloResponse>();
        if(rsp) {
            SYLAR_LOG_INFO(g_logger) << "recv " << sylar::PBToJsonString(*rsp);
        } else {
            break;
        }
    }

    test::HelloResponse rsp;
    rsp.set_id("hello");
    rsp.set_msg("world");

    response->setAsPB(rsp);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamA2 over";
    return 0;
}



int32_t HandleHelloServiceHelloStreamB2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::grpc::GrpcStreamClient::ptr cli,
                                sylar::http2::Http2Session::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto req = request->getAsPB<test::HelloRequest>();
    if(!req) {
        response->setResult(100);
        response->setError("invalid pb");
        return -1;
    }
    SYLAR_LOG_INFO(g_logger) << "---" << sylar::PBToJsonString(*req) << " - " << req;

    for(int i = 0; i < 10; ++i) {
        test::HelloResponse rsp;
        rsp.set_id("hello");
        rsp.set_msg("world");
        cli->sendMessage(rsp);
        sleep(1);
    }
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB2 over";
    return 0;
}

int32_t HandleHelloServiceHelloStreamC2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResult::ptr response,
                                sylar::grpc::GrpcStreamClient::ptr cli,
                                sylar::http2::Http2Session::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "stream_id = " << request->getRequest()->getStreamId();
    SYLAR_LOG_INFO(g_logger) << *request->getRequest();
    auto wg = sylar::WorkerGroup::Create(1);
    wg->schedule([cli](){
        for(int i = 0; i < 5; ++i) {
            test::HelloResponse rsp;
            rsp.set_id("hello");
            rsp.set_msg("world");
            cli->sendMessage(rsp);
            sleep(1);
        }
    });
    //while(true) {
    for(int i = 0; i < 5; ++i) {
        auto rsp = cli->recvMessage<test::HelloResponse>();
        if(!rsp) {
            break;
        }
        SYLAR_LOG_INFO(g_logger) << "recv " << sylar::PBToJsonString(*rsp);
    }
    wg->waitAll();
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC2 over";
    return 0;
}


std::string bigstr(10, 'a');

class MyModule : public sylar::RockModule {
public:
    MyModule()
        :RockModule("hello", "1.0", "") {
        //sylar::Singleton<A>::GetInstance();
    }

    bool onLoad() override {
        sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onLoad------------" << std::endl;
        return true;
    }

    bool onUnload() override {
        sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onUnload------------" << std::endl;
        return true;
    }

    bool onServerReady() {
        std::vector<sylar::TcpServer::ptr> svrs;
        sylar::Application::GetInstance()->getServer("http2", svrs);
        for(auto& i : svrs) {
            auto h2 = std::dynamic_pointer_cast<sylar::http2::Http2Server>(i);
            auto slt = h2->getServletDispatch();
            slt->addServlet("/test", HandleTest);

            h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
                    , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::UNARY, HandleHelloServiceHello, nullptr));

            h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
                    , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::CLIENT, HandleHelloServiceHelloStreamA, HandleHelloServiceHelloStreamA2));
            h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
                    , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::SERVER, HandleHelloServiceHelloStreamB, HandleHelloServiceHelloStreamB2));
            h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
                    , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::BOTH, HandleHelloServiceHelloStreamC, HandleHelloServiceHelloStreamC2));
        }
        registerService("rock", "sylar.top", "blog");
        auto rpy = sylar::RedisUtil::Cmd("local", "get abc");
        if(!rpy) {
            SYLAR_LOG_ERROR(g_logger) << "redis cmd get abc error";
        } else {
            SYLAR_LOG_ERROR(g_logger) << "redis get abc: "
                << (rpy->str ? rpy->str : "(null)");
        }

        /*
        sylar::IOManager::GetThis()->addTimer(1000, [](){
            // test Hello
            auto lb = sylar::Application::GetInstance()->getGrpcSDLoadBalance();
            test::HelloRequest hr;
            hr.set_id(bigstr + "hello_" + std::to_string(time(0)));
            hr.set_msg(bigstr + "world_" + std::to_string(time(0)));
            auto rt = lb->request("grpc", "test", "/test.HelloService/Hello", hr, 1000);
            SYLAR_LOG_INFO(g_logger) << rt->toString();
            if(rt->getResponse()) {
                SYLAR_LOG_INFO(g_logger) << *rt->getResponse();
            }
        }, true);
        */

        /*
        auto lb = sylar::Application::GetInstance()->getGrpcSDLoadBalance();
        test::HelloRequest hr;
        hr.set_id(bigstr + "hello_" + std::to_string(time(0)));
        hr.set_msg(bigstr + "world_" + std::to_string(time(0)));

        sylar::http::HttpRequest::ptr req = std::make_shared<sylar::http::HttpRequest>(0x20, false);
        req->setMethod(sylar::http::HttpMethod::POST);
        //req->setPath("/test.HelloService/HelloStreamA");
        //req->setPath("/test.HelloService/HelloStreamC");
        req->setPath("/test.HelloService/HelloStreamB");
        req->setHeader("content-type", "application/grpc+proto");

        auto cli = lb->openStreamClient("grpc", "test", req);
        SYLAR_LOG_INFO(g_logger) << "=====" << cli;
        sylar::IOManager::GetThis()->addTimer(1000, [cli](){
            test::HelloRequest hr;
            hr.set_id(bigstr + "hello_" + std::to_string(time(0)));
            hr.set_msg(bigstr + "world_" + std::to_string(time(0)));
            cli->sendMessage(hr);
        }, false);
        sylar::IOManager::GetThis()->schedule([cli](){
            while(true) {
                auto rsp = cli->recvMessage<test::HelloResponse>();
                if(rsp) {
                    SYLAR_LOG_INFO(g_logger) << "===" << sylar::PBToJsonString(*rsp);
                } else {
                    SYLAR_LOG_INFO(g_logger) << "out";
                    break;
                }
            }
        });
        */

        return true;
    }

    bool handleRockRequest(sylar::RockRequest::ptr request
                        ,sylar::RockResponse::ptr response
                        ,sylar::RockStream::ptr stream) {
        //SYLAR_LOG_INFO(g_logger) << "handleRockRequest " << request->toString();
        //sleep(1);
        response->setResult(0);
        response->setResultStr("ok");
        response->setBody("echo: " + request->getBody());

        usleep(100 * 1000);
        auto addr = stream->getLocalAddressString();
        if(addr.find("8061") != std::string::npos) {
            if(rand() % 100 < 50) {
                usleep(10 * 1000);
            } else if(rand() % 100 < 10) {
                response->setResult(-1000);
            }
        } else {
            //if(rand() % 100 < 25) {
            //    usleep(10 * 1000);
            //} else if(rand() % 100 < 10) {
            //    response->setResult(-1000);
            //}
        }
        return true;
        //return rand() % 100 < 90;
    }

    bool handleRockNotify(sylar::RockNotify::ptr notify 
                        ,sylar::RockStream::ptr stream) {
        SYLAR_LOG_INFO(g_logger) << "handleRockNotify " << notify->toString();
        return true;
    }

};

extern "C" {

sylar::Module* CreateModule() {
    sylar::Singleton<A>::GetInstance();
    std::cout << "=============CreateModule=================" << std::endl;
    return new MyModule;
}

void DestoryModule(sylar::Module* ptr) {
    std::cout << "=============DestoryModule=================" << std::endl;
    delete ptr;
}

}
