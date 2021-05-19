#include "sylar/http2/http2_server.h"

void run() {
    sylar::http2::Http2Server::ptr server(new sylar::http2::Http2Server);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("127.0.0.1:8099");
    //server->bind(addr, true);
    //server->loadCertificates("rootCA.pem", "rootCA.key");

    server->bind(addr, false);
    server->start();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.schedule(run);
    iom.stop();
    return 0;
}
