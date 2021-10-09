#include "http2_protocol.h"
#include "sylar/log.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static const std::vector<std::string> s_http2error_strings = {
    "OK",
    "PROTOCOL_ERROR",
    "INTERNAL_ERROR",
    "FLOW_CONTROL_ERROR",
    "SETTINGS_TIMEOUT_ERROR",
    "STREAM_CLOSED_ERROR",
    "FRAME_SIZE_ERROR",
    "REFUSED_STREAM_ERROR",
    "CANCEL_ERROR",
    "COMPRESSION_ERROR",
    "CONNECT_ERROR",
    "ENHANCE_YOUR_CALM_ERROR",
    "INADEQUATE_SECURITY_ERROR",
    "HTTP11_REQUIRED_ERROR",
};

std::string Http2ErrorToString(Http2Error error) {
    static uint32_t SIZE = s_http2error_strings.size();
    uint8_t v = (uint8_t)error;
    if(v < SIZE) {
        return s_http2error_strings[v];
    }
    return "UNKNOW(" + std::to_string((uint32_t)v) + ")";
}

std::string Http2Settings::toString() const {
    std::stringstream ss;
    ss << "[Http2Settings header_table_size=" << header_table_size
       << " max_header_list_size=" << max_header_list_size
       << " max_concurrent_streams=" << max_concurrent_streams
       << " max_frame_size=" << max_frame_size
       << " initial_window_size=" << initial_window_size
       << " enable_push=" << enable_push << "]";
    return ss.str();
}

void Http2InitRequestForWrite(sylar::http::HttpRequest::ptr req, bool ssl) {
    req->setHeader(":scheme", (ssl ? "https" : "http"));
    if(!req->hasHeader(":path", nullptr)) {
        req->setHeader(":path", req->getUri());
    }
    req->setHeader(":method", http::HttpMethodToString(req->getMethod()));
}

void Http2InitResponseForWrite(sylar::http::HttpResponse::ptr rsp) {
    rsp->setHeader(":status", std::to_string((uint32_t)rsp->getStatus()));
}

void Http2InitRequestForRead(sylar::http::HttpRequest::ptr req) {
    req->setMethod(http::StringToHttpMethod(req->getHeader(":method")));
    if(req->hasHeader(":path", nullptr)) {
        req->setUri(req->getHeader(":path"));
        //SYLAR_LOG_INFO(g_logger) << req->getPath() << " - " << req->getQuery() << " - " << req->getFragment();
    }
}

void Http2InitResponseForRead(sylar::http::HttpResponse::ptr rsp) {
    rsp->setStatus((http::HttpStatus)sylar::TypeUtil::Atoi(rsp->getHeader(":status")));
}

}
}
