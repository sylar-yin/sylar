#include "sylar/log.h"
#include "tests/test.HelloService.HelloStreamA.grpc.h"

namespace test {
namespace HelloService {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int32_t HelloStreamAServlet::handle(ServerStream::ptr stream, RspPtr rsp) {
	SYLAR_LOG_WARN(g_logger) << "Unhandle test.HelloService.HelloStreamA";
	m_response->setResult(404);
	m_response->setError("Unhandle");
	return 0;
}

} //namespace HelloService
} //namespace test
