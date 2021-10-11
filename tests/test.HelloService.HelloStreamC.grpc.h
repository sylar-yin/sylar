#ifndef __TEST_HELLOSERVICE_HELLOSTREAMC_GRPC_H__
#define __TEST_HELLOSERVICE_HELLOSTREAMC_GRPC_H__

#include "sylar/grpc/grpc_servlet.h"
#include "test.pb.h"

namespace test {
namespace HelloService {

class HelloStreamCServlet : public sylar::grpc::GrpcStreamBidirectionServlet<test::HelloRequest, test::HelloResponse> {
public:
	int32_t handle(ServerStream::ptr stream) override;
	GRPC_SERVLET_INIT_NAME(HelloStreamCServlet, "/test.HelloService/HelloStreamC");

}; //class HelloStreamCServlet

} //namespace HelloService
} //namespace test

#endif
