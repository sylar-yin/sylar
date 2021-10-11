#ifndef __TEST_HELLOSERVICE_HELLO_GRPC_H__
#define __TEST_HELLOSERVICE_HELLO_GRPC_H__

#include "sylar/grpc/grpc_servlet.h"
#include "test.pb.h"

namespace test {
namespace HelloService {

class HelloServlet : public sylar::grpc::GrpcUnaryServlet<test::HelloRequest, test::HelloResponse> {
public:
	int32_t handle(ReqPtr req, RspPtr rsp) override;
	GRPC_SERVLET_INIT_NAME(HelloServlet, "/test.HelloService/Hello");

}; //class HelloServlet

} //namespace HelloService
} //namespace test

#endif
