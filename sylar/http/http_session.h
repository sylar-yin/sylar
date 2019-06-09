#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "sylar/socket_stream.h"
#include "http.h"

namespace sylar {
namespace http {

class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);
};

}
}

#endif
