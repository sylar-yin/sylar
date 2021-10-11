#ifndef __TEST_HELLOSERVICE_GRPC_CC__
#define __TEST_HELLOSERVICE_GRPC_CC__

#include "sylar/log.h"
#include "tests/test.HelloService.Hello.grpc.h"
#include "tests/test.HelloService.HelloStreamA.grpc.h"
#include "tests/test.HelloService.HelloStreamB.grpc.h"
#include "tests/test.HelloService.HelloStreamC.grpc.h"
#include "tests/test.HelloService.grpc.h"

namespace test {
namespace HelloService {


static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void RegisterService(sylar::grpc::GrpcServer::ptr server) {
	server->addGrpcServlet("/test.HelloService/Hello", std::make_shared<HelloServlet>());
	SYLAR_LOG_INFO(g_logger) << "register grpc /test.HelloService/Hello";
	server->addGrpcServlet("/test.HelloService/HelloStreamA", std::make_shared<HelloStreamAServlet>());
	SYLAR_LOG_INFO(g_logger) << "register grpc /test.HelloService/HelloStreamA";
	server->addGrpcServlet("/test.HelloService/HelloStreamB", std::make_shared<HelloStreamBServlet>());
	SYLAR_LOG_INFO(g_logger) << "register grpc /test.HelloService/HelloStreamB";
	server->addGrpcServlet("/test.HelloService/HelloStreamC", std::make_shared<HelloStreamCServlet>());
	SYLAR_LOG_INFO(g_logger) << "register grpc /test.HelloService/HelloStreamC";
}

} //namespace HelloService
} //namespace test

#endif
