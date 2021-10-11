#include "sylar/log.h"
#include "tests/test.HelloService.HelloStreamC.grpc.h"

namespace test {
namespace HelloService {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int32_t HelloStreamCServlet::handle(ServerStream::ptr stream) {
	SYLAR_LOG_WARN(g_logger) << "Unhandle test.HelloService.HelloStreamC";
	m_response->setResult(404);
	m_response->setError("Unhandle");
	return 0;
}

} //namespace HelloService
} //namespace test
