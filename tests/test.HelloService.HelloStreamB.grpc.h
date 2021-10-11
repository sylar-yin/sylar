#ifndef __TEST_HELLOSERVICE_HELLOSTREAMB_GRPC_H__
#define __TEST_HELLOSERVICE_HELLOSTREAMB_GRPC_H__

#include "sylar/grpc/grpc_servlet.h"
#include "test.pb.h"

namespace test {
namespace HelloService {

class HelloStreamBServlet : public sylar::grpc::GrpcStreamServerServlet<test::HelloRequest, test::HelloResponse> {
public:
	int32_t handle(ServerStream::ptr stream, ReqPtr req) override;
	GRPC_SERVLET_INIT_NAME(HelloStreamBServlet, "/test.HelloService/HelloStreamB");

}; //class HelloStreamBServlet

} //namespace HelloService
} //namespace test

#endif
