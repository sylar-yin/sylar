#ifndef __SYLAR_HTTP2_HTTP2_PROTOCOL_H__
#define __SYLAR_HTTP2_HTTP2_PROTOCOL_H__

#include "sylar/http/http.h"

namespace sylar {
namespace http2 {

static const uint32_t DEFAULT_MAX_FRAME_SIZE = 16384;
static const uint32_t MAX_MAX_FRAME_SIZE = 0xFFFFFF;
static const uint32_t DEFAULT_HEADER_TABLE_SIZE = 4096;
static const uint32_t DEFAULT_MAX_HEADER_LIST_SIZE = 0x400000;
static const uint32_t DEFAULT_INITIAL_WINDOW_SIZE = 65535;
static const uint32_t MAX_INITIAL_WINDOW_SIZE = 0x7FFFFFFF;
static const uint32_t DEFAULT_MAX_READ_FRAME_SIZE = 1 << 20;
static const uint32_t DEFAULT_MAX_CONCURRENT_STREAMS = 0xffffffffu;

enum class Http2Error {
    OK                          = 0x0,
    PROTOCOL_ERROR              = 0x1,
    INTERNAL_ERROR              = 0x2,
    FLOW_CONTROL_ERROR          = 0x3,
    SETTINGS_TIMEOUT_ERROR      = 0x4,
    STREAM_CLOSED_ERROR         = 0x5,
    FRAME_SIZE_ERROR            = 0x6,
    REFUSED_STREAM_ERROR        = 0x7,
    CANCEL_ERROR                = 0x8,
    COMPRESSION_ERROR           = 0x9,
    CONNECT_ERROR               = 0xa,
    ENHANCE_YOUR_CALM_ERROR     = 0xb,
    INADEQUATE_SECURITY_ERROR   = 0xc,
    HTTP11_REQUIRED_ERROR       = 0xd,
};

std::string Http2ErrorToString(Http2Error error);

struct Http2Settings {
    uint32_t header_table_size = DEFAULT_HEADER_TABLE_SIZE;
    uint32_t max_header_list_size = DEFAULT_MAX_HEADER_LIST_SIZE;
    uint32_t max_concurrent_streams = DEFAULT_MAX_CONCURRENT_STREAMS;
    uint32_t max_frame_size = DEFAULT_MAX_FRAME_SIZE;
    uint32_t initial_window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    bool enable_push = 0;

   std::string toString() const;
};

void Http2InitRequestForWrite(sylar::http::HttpRequest::ptr req, bool ssl = false);
void Http2InitResponseForWrite(sylar::http::HttpResponse::ptr rsp);

void Http2InitRequestForRead(sylar::http::HttpRequest::ptr req);
void Http2InitResponseForRead(sylar::http::HttpResponse::ptr rsp);

}
}

#endif
