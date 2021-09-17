#include "sylar/http2/http2_server.h"
#include "sylar/http/http_server.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void run() {
    sylar::http2::Http2Server::ptr server(new sylar::http2::Http2Server);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8099");
    //server->bind(addr, true);
    //server->loadCertificates("rootCA.pem", "rootCA.key");

    auto sd = server->getServletDispatch();
    sd->addServlet("/hello", 
    [](sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) {
        
        SYLAR_LOG_INFO(g_logger) << *request;

        response->setBody("hello http2");
        return 0;
    });


    server->bind(addr, false);
    //server->bind(addr, true);
    //server->loadCertificates("server.crt", "server.key");
    server->start();
}

void run2() {
    sylar::http::HttpServer::ptr server(new sylar::http::HttpServer);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8090");
    server->bind(addr, true);
    server->loadCertificates("server.crt", "server.key");
    server->start();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.schedule(run);
    iom.schedule(run2);
    iom.stop();
    return 0;
}
