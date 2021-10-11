#ifndef __TEST_HELLOSERVICE_GRPC_H__
#define __TEST_HELLOSERVICE_GRPC_H__

#include "sylar/grpc/grpc_server.h"

namespace test {
namespace HelloService {

void RegisterService(sylar::grpc::GrpcServer::ptr server);

} //namespace HelloService
} //namespace test

#endif
