#include "sylar/grpc/grpc_stream.h"
#include "tests/test.pb.h"
#include "sylar/log.h"
#include "sylar/sylar.h"
#include "sylar/grpc/grpc_connection.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void testUnary(sylar::grpc::GrpcConnection::ptr conn) {
    std::string  prefx = "aaaa";
    for(int i = 0; i < 4; ++i) {
        prefx = prefx + prefx;
    }

    for(int x = 0; x < 20; ++x) {
        sylar::IOManager::GetThis()->schedule([conn, x, prefx](){
            int fail = 0;
            int error = 0;
            static int sn = 0;
            for(int i = 0; i < 20; ++i) {
                auto hr = std::make_shared<test::HelloRequest>();
                auto tmp = sylar::Atomic::addFetch(sn);
                hr->set_id(prefx + "hello_" + std::to_string(tmp));
                hr->set_msg(prefx + "world_" + std::to_string(tmp));
                auto rsp = conn->request("/test.HelloService/Hello", hr, 100000);
                SYLAR_LOG_INFO(g_logger) << rsp->toString() << std::endl;
                if(rsp->getResponse()) {
                    //std::cout << *rsp->getResponse() << std::endl;

                    auto hrp = rsp->getAsPB<test::HelloResponse>();
                    if(hrp) {
                        //std::cout << sylar::PBToJsonString(*hrp) << std::endl;
                        SYLAR_LOG_INFO(g_logger) << "========" << rsp->getResult() << " - " << sylar::PBToJsonString(*hr);
                    } else { 
                        SYLAR_LOG_INFO(g_logger) << "########" << rsp->getResult() << " - " << sylar::PBToJsonString(*hr);
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

void testStreamClient(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto stm = conn->openGrpcStream("/test.HelloService/HelloStreamA");
            //sleep(1);
            for(int i = 0; i < 10; ++i) {
                auto hr = std::make_shared<test::HelloRequest>();
                hr->set_id("hello_stream_a");
                hr->set_msg("world_stream_a");

                stm->sendMessage(hr);
            }
            stm->sendData("", true);
            auto rsp = stm->recvMessage<test::HelloResponse>();
            SYLAR_LOG_INFO(g_logger) << "===== HelloStreamA - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
            //sleep(1);
        }
        sleep(12);
    }
}

void testStreamClient2(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto stm = conn->openGrpcClientStream<test::HelloRequest, test::HelloResponse>("/test.HelloService/HelloStreamA");
            //sleep(1);
            for(int i = 0; i < 10; ++i) {
                auto hr = std::make_shared<test::HelloRequest>();
                hr->set_id("hello_stream_a");
                hr->set_msg("world_stream_a");

                if(stm->send(hr) <= 0) {
                    SYLAR_LOG_INFO(g_logger) << "testStreamClient2 send fail";
                    break;
                }
            }
            auto rsp = stm->closeAndRecv();
            SYLAR_LOG_INFO(g_logger) << "===== testStreamClient2 HelloStreamA - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
            //sleep(1);
        }
        sleep(12);
    }
}

void testStreamServer(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto stm = conn->openGrpcStream("/test.HelloService/HelloStreamB");
            auto hr = std::make_shared<test::HelloRequest>();
            hr->set_id("hello_stream_b");
            hr->set_msg("world_stream_b");
            stm->sendMessage(hr, true);
            while(true) {
                auto rsp = stm->recvMessage<test::HelloResponse>();
                if(rsp) {
                    SYLAR_LOG_INFO(g_logger) << "===== HelloStreamB - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
                } else {
                    SYLAR_LOG_INFO(g_logger) << "===== HelloStreamB Out";
                    break;
                }
            }
            //sleep(1);
        }
        sleep(12);
    }
}

void testStreamServer2(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto hr = std::make_shared<test::HelloRequest>();
            hr->set_id("hello_stream_b");
            hr->set_msg("world_stream_b");
            auto stm = conn->openGrpcServerStream<test::HelloRequest, test::HelloResponse>("/test.HelloService/HelloStreamB", hr);
            while(true) {
                auto rsp = stm->recv();
                if(rsp) {
                    SYLAR_LOG_INFO(g_logger) << "testStreamServer2 ===== HelloStreamB - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
                } else {
                    SYLAR_LOG_INFO(g_logger) << "testStreamServer2 ===== HelloStreamB Out";
                    break;
                }
            }
            //sleep(1);
        }
        sleep(12);
    }
}

void testStreamBidi(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto stm = conn->openGrpcStream("/test.HelloService/HelloStreamC");
            auto wg = sylar::WorkerGroup::Create(1);
            wg->schedule([stm](){
                for(int i = 0; i< 10; ++i) {
                    auto hr = std::make_shared<test::HelloRequest>();
                    hr->set_id("hello_stream_c");
                    hr->set_msg("world_stream_c");
                    stm->sendMessage(hr);
                }
                stm->sendData("", true);
            });
            while(true) {
                auto rsp = stm->recvMessage<test::HelloResponse>();
                if(rsp) {
                    SYLAR_LOG_INFO(g_logger) << "===== HelloStreamC - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
                } else {
                    SYLAR_LOG_INFO(g_logger) << "===== HelloStreamC Out";
                    break;
                }
            }
            wg->waitAll();
            sleep(5);
            SYLAR_LOG_INFO(g_logger) << "-------------------------------";
        }
        sleep(12);
    }
}

void testStreamBidi2(sylar::grpc::GrpcConnection::ptr conn) {
    while(true) {
        for(int x = 0; x < 100; ++x) {
            auto stm = conn->openGrpcBidirectionStream<test::HelloRequest, test::HelloResponse>("/test.HelloService/HelloStreamC");
            auto wg = sylar::WorkerGroup::Create(1);
            wg->schedule([stm](){
                for(int i = 0; i< 10; ++i) {
                    auto hr = std::make_shared<test::HelloRequest>();
                    hr->set_id("hello_stream_c");
                    hr->set_msg("world_stream_c");
                    if(stm->send(hr) <= 0) {
                        SYLAR_LOG_INFO(g_logger) << "testStreamBidi2 send fail";
                        break;
                    }
                }
                sleep(1);
                stm->close();
            });
            while(true) {
                auto rsp = stm->recv();
                if(rsp) {
                    SYLAR_LOG_INFO(g_logger) << "testStreamBidi2 ===== HelloStreamC - " << rsp << " - " << (rsp ? sylar::PBToJsonString(*rsp) : "(null)");
                } else {
                    SYLAR_LOG_INFO(g_logger) << "testStreamBidi2 ===== HelloStreamC Out";
                    break;
                }
            }
            wg->waitAll();
            sleep(5);
            SYLAR_LOG_INFO(g_logger) << "-------------------------------";
        }
        sleep(12);
    }
}

void run() {
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("10.104.26.23:8099");
    sylar::grpc::GrpcConnection::ptr conn(new sylar::grpc::GrpcConnection);

    if(!conn->connect(addr, false)) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " fail" << std::endl;
        return;
    }
    conn->start();
    //sleep(1);
    sylar::IOManager::GetThis()->addTimer(10000, [conn](){
            SYLAR_LOG_INFO(g_logger) << "------ status ------";
            SYLAR_LOG_INFO(g_logger) << conn->isConnected();
            SYLAR_LOG_INFO(g_logger) << conn->recving;
            SYLAR_LOG_INFO(g_logger) << "------ status ------";
    }, true);
    //sleep(5);
    //sylar::IOManager::GetThis()->schedule(std::bind(testUnary, conn));
    //for(int i = 0; i < 1000; ++i) {
    //    //sylar::IOManager::GetThis()->schedule(std::bind(testStreamClient, conn));
    //    //sylar::IOManager::GetThis()->schedule(std::bind(testStreamServer, conn));
    //    sylar::IOManager::GetThis()->schedule(std::bind(testStreamBidi, conn));
    //    sleep(10);
    //}

    //testStreamBidi(conn);
    //testStreamClient2(conn);
    //testStreamServer2(conn);
    testStreamBidi2(conn);
}

int main(int argc, char** argv) {
    sylar::IOManager io(2);
    io.schedule(run);
    io.addTimer(10000, [](){}, true);
    io.stop();
    return 0;
}
