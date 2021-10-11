#ifndef __TEST_HELLOSERVICE_HELLOSTREAMA_GRPC_H__
#define __TEST_HELLOSERVICE_HELLOSTREAMA_GRPC_H__

#include "sylar/grpc/grpc_servlet.h"
#include "test.pb.h"

namespace test {
namespace HelloService {

class HelloStreamAServlet : public sylar::grpc::GrpcStreamClientServlet<test::HelloRequest, test::HelloResponse> {
public:
	int32_t handle(ServerStream::ptr stream, RspPtr rsp) override;
	GRPC_SERVLET_INIT_NAME(HelloStreamAServlet, "/test.HelloService/HelloStreamA");

}; //class HelloStreamAServlet

} //namespace HelloService
} //namespace test

#endif
