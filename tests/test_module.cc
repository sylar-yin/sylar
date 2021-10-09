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

int32_t HandleTest2(std::shared_ptr<test::HelloRequest> req, std::shared_ptr<test::HelloResponse> rsp) {
    SYLAR_LOG_INFO(g_logger) << "HandleTest2 " << sylar::PBToJsonString(*req);
    rsp->set_id("hello");
    rsp->set_msg("world");
    return 0;
}

int32_t HandleTest2Full(std::shared_ptr<test::HelloRequest> req, std::shared_ptr<test::HelloResponse> rsp,
                        sylar::grpc::GrpcRequest::ptr request, sylar::grpc::GrpcResponse::ptr response,
                        sylar::grpc::GrpcSession::ptr session) {
    SYLAR_LOG_INFO(g_logger) << "HandleTest2Full " << sylar::PBToJsonString(*req);
    rsp->set_id("hello");
    rsp->set_msg("world");
    return 0;
}

//class HelloServiceHello : public sylar::grpc::GrpcUnaryServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHello()
//        :Base("HelloServiceHello") {
//    }
//
//    int32_t handle(ReqPtr req, RspPtr rsp) {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHello req=" << sylar::PBToJsonString(*req);
//        rsp->set_id("hello");
//        rsp->set_msg("world");
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHello rsp=" << sylar::PBToJsonString(*rsp);
//        return 0;
//    }
//};
//
//class HelloServiceHelloFull : public sylar::grpc::GrpcUnaryFullServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloFull()
//        :Base("HelloServiceHelloFull") {
//    }
//
//    int32_t handle(ReqPtr req, RspPtr rsp, sylar::grpc::GrpcRequest::ptr request
//                  ,sylar::grpc::GrpcResponse::ptr response, sylar::grpc::GrpcSession::ptr session) {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloFull req=" << sylar::PBToJsonString(*req);
//        rsp->set_id("hello");
//        rsp->set_msg("world");
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloFull rsp=" << sylar::PBToJsonString(*rsp);
//        return 0;
//    }
//};



int32_t HandleHelloServiceHello(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResponse::ptr response,
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
                                sylar::grpc::GrpcResponse::ptr response,
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
    auto h2session = std::dynamic_pointer_cast<sylar::grpc::GrpcSession>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStream::ptr cli = std::make_shared<sylar::grpc::GrpcStream>(stream);

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
                                sylar::grpc::GrpcResponse::ptr response,
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
    auto h2session = std::dynamic_pointer_cast<sylar::grpc::GrpcSession>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStream::ptr cli = std::make_shared<sylar::grpc::GrpcStream>(stream);

    stream->sendResponse(response->getResponse(), false, true);

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
    }, true, true);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB over";
    return 0;
}

int32_t HandleHelloServiceHelloStreamC(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResponse::ptr response,
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
    auto h2session = std::dynamic_pointer_cast<sylar::grpc::GrpcSession>(session);

    auto stream = h2session->getStream(stream_id);
    sylar::grpc::GrpcStream::ptr cli = std::make_shared<sylar::grpc::GrpcStream>(stream);

    stream->sendResponse(response->getResponse(), false, true);

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
    }, true, true);
    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC over";
    return 0;
}

//class HelloServiceHelloStreamA : public sylar::grpc::GrpcStreamClientServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamA():Base("HelloServiceHelloStreamA") { }
//
//    int32_t handle(typename Reader::ptr reader, RspPtr rsp) {
//        while(true) {
//            auto req = reader->recvMessage();
//            if(req) {
//                SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamA recv " << sylar::PBToJsonString(*req);
//            } else {
//                break;
//            }
//        }
//        rsp->set_id("HelloServiceHelloStreamA");
//        rsp->set_msg("world");
//        return 0;
//    }
//};
//
//class HelloServiceHelloStreamAFull : public sylar::grpc::GrpcStreamClientFullServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamAFull():Base("HelloServiceHelloStreamAFull") { }
//
//    int32_t handle(typename Reader::ptr reader, RspPtr rsp,
//                           sylar::grpc::GrpcRequest::ptr request,
//                           sylar::grpc::GrpcResponse::ptr response,
//                           sylar::grpc::GrpcStreamClient::ptr stream,
//                           sylar::grpc::GrpcSession::ptr session) {
//        while(true) {
//            auto req = reader->recvMessage();
//            if(req) {
//                SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamAFull recv " << sylar::PBToJsonString(*req);
//            } else {
//                break;
//            }
//        }
//        rsp->set_id("HelloServiceHelloStreamAFull");
//        rsp->set_msg("world");
//        return 0;
//    }
//};



//int32_t HandleServiceHelloStreamA3(sylar::grpc::GrpcStreamReader<test::HelloRequest>::ptr reader, std::shared_ptr<test::HelloResponse> rsp) {
//    while(true) {
//        auto req = reader->recvMessage();
//        if(req) {
//            SYLAR_LOG_INFO(g_logger) << "HandleServiceHelloStreamA3 recv " << sylar::PBToJsonString(*req);
//        } else {
//            break;
//        }
//    }
//    rsp->set_id("HandleServiceHelloStreamA3");
//    rsp->set_msg("world");
//    return 0;
//}
//
//int32_t HandleServiceHelloStreamA3Full(sylar::grpc::GrpcStreamReader<test::HelloRequest>::ptr reader, std::shared_ptr<test::HelloResponse> rsp,
//                   sylar::grpc::GrpcRequest::ptr request,
//                   sylar::grpc::GrpcResponse::ptr response,
//                   sylar::grpc::GrpcStream::ptr stream,
//                   sylar::grpc::GrpcSession::ptr session) {
//    while(true) {
//        auto req = reader->recvMessage();
//        if(req) {
//            SYLAR_LOG_INFO(g_logger) << "HandleServiceHelloStreamA3Full recv " << sylar::PBToJsonString(*req);
//        } else {
//            break;
//        }
//    }
//    rsp->set_id("HandleServiceHelloStreamA3Full");
//    rsp->set_msg("world");
//    SYLAR_LOG_INFO(g_logger) << "HandleServiceHelloStreamA3Full rsp " << sylar::PBToJsonString(*rsp);
//    return 0;
//}

int32_t HandleHelloServiceHelloStreamA2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResponse::ptr response,
                                sylar::grpc::GrpcStream::ptr cli,
                                sylar::grpc::GrpcSession::ptr session) {
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

//class HelloServiceHelloStreamB : public sylar::grpc::GrpcStreamServerServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamB()
//        :Base("HelloServiceHelloStreamB") {
//    }
//    int32_t handle(ReqPtr req, typename Writer::ptr writer) {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamB " << sylar::PBToJsonString(*req);
//        for(int i = 0; i < 10; ++i) {
//            auto rsp = std::make_shared<test::HelloResponse>();
//            rsp->set_id("hello");
//            rsp->set_msg("world");
//            writer->sendMessage(rsp);
//            sleep(1);
//        }
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamB over";
//        return 0;
//    }
//};
//
//class HelloServiceHelloStreamBFull : public sylar::grpc::GrpcStreamServerFullServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamBFull()
//        :Base("HelloServiceHelloStreamBFull") {
//    }
//    int32_t handle(ReqPtr req, typename Writer::ptr writer,
//                   sylar::grpc::GrpcRequest::ptr request,
//                   sylar::grpc::GrpcResponse::ptr response,
//                   sylar::grpc::GrpcStreamClient::ptr stream,
//                   sylar::grpc::GrpcSession::ptr session) override {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamBFull " << sylar::PBToJsonString(*req);
//        for(int i = 0; i < 10; ++i) {
//            auto rsp = std::make_shared<test::HelloResponse>();
//            rsp->set_id("hello");
//            rsp->set_msg("world");
//            writer->sendMessage(rsp);
//            sleep(1);
//        }
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamBFull over";
//        return 0;
//    }
//};

//int32_t HandleHelloServiceHelloStreamB3(std::shared_ptr<test::HelloRequest> req, sylar::grpc::GrpcStreamWriter<test::HelloResponse>::ptr writer) {
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB3 " << sylar::PBToJsonString(*req);
//
//    for(int i = 0; i < 10; ++i) {
//        auto rsp = std::make_shared<test::HelloResponse>();
//        rsp->set_id("hello");
//        rsp->set_msg("world");
//        writer->sendMessage(rsp);
//        sleep(1);
//    }
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB3 over";
//    return 0;
//}
//
//int32_t HandleHelloServiceHelloStreamB3Full(std::shared_ptr<test::HelloRequest> req, sylar::grpc::GrpcStreamWriter<test::HelloResponse>::ptr writer,
//           sylar::grpc::GrpcRequest::ptr request,
//           sylar::grpc::GrpcResponse::ptr response,
//           sylar::grpc::GrpcStream::ptr stream,
//           sylar::grpc::GrpcSession::ptr session) {
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB3Full " << sylar::PBToJsonString(*req);
//
//    for(int i = 0; i < 10; ++i) {
//        auto rsp = std::make_shared<test::HelloResponse>();
//        rsp->set_id("hello");
//        rsp->set_msg("world");
//        writer->sendMessage(rsp);
//        //sleep(1);
//    }
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamB3Full over";
//    return 0;
//}

int32_t HandleHelloServiceHelloStreamB2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResponse::ptr response,
                                sylar::grpc::GrpcStream::ptr cli,
                                sylar::grpc::GrpcSession::ptr session) {
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

//class HelloServiceHelloStreamC : public sylar::grpc::GrpcStreamBothServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamC()
//        :Base("HelloServiceHelloStreamC") {
//    }
//
//    int32_t handle(typename ReaderWriter::ptr rw) {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamC::handle";
//        auto wg = sylar::WorkerGroup::Create(1);
//        wg->schedule([rw](){
//            for(int i = 0; i < 5; ++i) {
//                auto rsp = std::make_shared<test::HelloResponse>();
//                rsp->set_id("hello");
//                rsp->set_msg("world");
//                rw->sendMessage(rsp);
//                sleep(1);
//            }
//        });
//        //while(true) {
//        for(int i = 0; i < 5; ++i) {
//            auto rsp = rw->recvMessage();
//            if(!rsp) {
//                break;
//            }
//            SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamC recv " << sylar::PBToJsonString(*rsp);
//        }
//        wg->waitAll();
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamC over";
//        return 0;
//    }
//};
//
//class HelloServiceHelloStreamCFull : public sylar::grpc::GrpcStreamBothFullServlet<test::HelloRequest, test::HelloResponse> {
//public:
//    HelloServiceHelloStreamCFull()
//        :Base("HelloServiceHelloStreamCFull") {
//    }
//
//    int32_t handle(typename ReaderWriter::ptr rw,
//                    sylar::grpc::GrpcRequest::ptr request,
//                    sylar::grpc::GrpcResponse::ptr response,
//                    sylar::grpc::GrpcStream::ptr stream,
//                    sylar::grpc::GrpcSession::ptr session) {
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamCFull::handle";
//        auto wg = sylar::WorkerGroup::Create(1);
//        wg->schedule([rw](){
//            for(int i = 0; i < 5; ++i) {
//                auto rsp = std::make_shared<test::HelloResponse>();
//                rsp->set_id("hello");
//                rsp->set_msg("world");
//                rw->sendMessage(rsp);
//                sleep(1);
//            }
//        });
//        //while(true) {
//        for(int i = 0; i < 5; ++i) {
//            auto rsp = rw->recvMessage();
//            if(!rsp) {
//                break;
//            }
//            SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamCFull recv " << sylar::PBToJsonString(*rsp);
//        }
//        wg->waitAll();
//        SYLAR_LOG_INFO(g_logger) << "HelloServiceHelloStreamCFull over";
//        return 0;
//    }
//};

//int32_t HandleHelloServiceHelloStreamC3(sylar::grpc::GrpcStreamSession<test::HelloRequest, test::HelloResponse>::ptr rw) {
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3";
//    auto wg = sylar::WorkerGroup::Create(1);
//    wg->schedule([rw](){
//        for(int i = 0; i < 5; ++i) {
//            auto rsp = std::make_shared<test::HelloResponse>();
//            rsp->set_id("hello");
//            rsp->set_msg("world");
//            rw->sendMessage(rsp);
//            sleep(1);
//        }
//    });
//    //while(true) {
//    for(int i = 0; i < 5; ++i) {
//        auto rsp = rw->recvMessage();
//        if(!rsp) {
//            break;
//        }
//        SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3 recv " << sylar::PBToJsonString(*rsp);
//    }
//    wg->waitAll();
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3 over";
//
//    return 0;
//}
//
//int32_t HandleHelloServiceHelloStreamC3Full(sylar::grpc::GrpcStreamSession<test::HelloRequest, test::HelloResponse>::ptr rw,
//                                            sylar::grpc::GrpcRequest::ptr request,
//                                            sylar::grpc::GrpcResponse::ptr response,
//                                            sylar::grpc::GrpcStream::ptr stream,
//                                            sylar::grpc::GrpcSession::ptr session) {
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3Full";
//    auto wg = sylar::WorkerGroup::Create(1);
//    wg->schedule([rw](){
//        for(int i = 0; i < 5; ++i) {
//            auto rsp = std::make_shared<test::HelloResponse>();
//            rsp->set_id("hello");
//            rsp->set_msg("world");
//            rw->sendMessage(rsp);
//            sleep(1);
//        }
//    });
//    //while(true) {
//    for(int i = 0; i < 5; ++i) {
//        auto rsp = rw->recvMessage();
//        if(!rsp) {
//            break;
//        }
//        SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3Full recv " << sylar::PBToJsonString(*rsp);
//    }
//    wg->waitAll();
//    SYLAR_LOG_INFO(g_logger) << "HandleHelloServiceHelloStreamC3Full over";
//
//    return 0;
//}

int32_t HandleHelloServiceHelloStreamC2(sylar::grpc::GrpcRequest::ptr request,
                                sylar::grpc::GrpcResponse::ptr response,
                                sylar::grpc::GrpcStream::ptr cli,
                                sylar::grpc::GrpcSession::ptr session) {
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
#if 0
        std::vector<sylar::TcpServer::ptr> svrs;
        sylar::Application::GetInstance()->getServer("http2", svrs);
        for(auto& i : svrs) {
            auto h2 = std::dynamic_pointer_cast<sylar::http2::Http2Server>(i);
            auto slt = h2->getServletDispatch();
            slt->addServlet("/test", HandleTest);

            h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
                    , std::make_shared<HelloServiceHelloFull>());

            //h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
            //        , sylar::grpc::GrpcUnaryFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleTest2));
            //h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
            //        , sylar::grpc::GrpcUnaryFullFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleTest2Full));
            //h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
            //        , std::make_shared<HelloServiceHello>());

            //h2->addGrpcServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "Hello")
            //        , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::UNARY, HandleHelloServiceHello, nullptr));

            //h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
            //        , std::make_shared<HelloServiceHelloStreamA>());
            //h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
            //        , std::make_shared<HelloServiceHelloStreamAFull>());

            h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
                    , sylar::grpc::GrpcStreamClientFullFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleServiceHelloStreamA3Full));
            //h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
            //        , sylar::grpc::GrpcStreamClientFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleServiceHelloStreamA3));
            //h2->addGrpcStreamClientServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamA")
            //        , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::CLIENT, HandleHelloServiceHelloStreamA, HandleHelloServiceHelloStreamA2));
            //h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
            //        , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::SERVER, HandleHelloServiceHelloStreamB, HandleHelloServiceHelloStreamB2));
            //h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
            //        , sylar::grpc::GrpcStreamServerFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleHelloServiceHelloStreamB3));
            h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
                    , sylar::grpc::GrpcStreamServerFullFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleHelloServiceHelloStreamB3Full));
            //h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
            //        , std::make_shared<HelloServiceHelloStreamBFull>());
            //h2->addGrpcStreamServerServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamB")
            //        , std::make_shared<HelloServiceHelloStreamB>());
            //h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
            //        , std::make_shared<HelloServiceHelloStreamC>());
            //h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
            //        , std::make_shared<HelloServiceHelloStreamCFull>());
            h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
                    , sylar::grpc::GrpcStreamBothFullFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleHelloServiceHelloStreamC3Full));
            //h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
            //        , sylar::grpc::GrpcStreamBothFunctionServlet<test::HelloRequest, test::HelloResponse>::Create(HandleHelloServiceHelloStreamC3));
            //h2->addGrpcStreamBothServlet(sylar::grpc::GrpcServlet::GetGrpcPath("test", "HelloService", "HelloStreamC")
            //        , sylar::grpc::GrpcFunctionServlet::Create(sylar::grpc::GrpcType::BOTH, HandleHelloServiceHelloStreamC, HandleHelloServiceHelloStreamC2));
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
#endif
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
