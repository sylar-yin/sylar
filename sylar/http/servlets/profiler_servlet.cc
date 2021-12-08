#include "profiler_servlet.h"
#include <gperftools/profiler.h>
#include <gperftools/heap-profiler.h>

#include "sylar/pack/pack.h"
#include "sylar/pack/json_encoder.h"

namespace sylar {
namespace http {

ProfilerServlet::ProfilerServlet()
    :Servlet("ProfilerServlet") {
}

struct _ProfilerStatus {
    bool cpu_profiler = false;
    bool mem_profiler = false;

    uint64_t cpu_start_time = 0;
    uint64_t mem_start_time = 0;

    std::string cpu_profiler_file;
    std::string mem_profiler_file;

    SYLAR_PACK(O(cpu_profiler, mem_profiler, cpu_start_time, mem_start_time, cpu_profiler_file, mem_profiler_file));
};

struct _RspStruct {
    int32_t code = 0;
    std::string msg;

    SYLAR_PACK(O(code, msg));
};

static _ProfilerStatus s_status;

int32_t handleStatus(sylar::http::HttpRequest::ptr request
                     , sylar::http::HttpResponse::ptr response
                     , sylar::SocketStream::ptr session) {
    response->setBody(sylar::pack::EncodeToJsonString(s_status, 0));
    return 0;
}

int32_t handleStart(sylar::http::HttpRequest::ptr request
                     , sylar::http::HttpResponse::ptr response
                     , sylar::SocketStream::ptr session) {
    auto type = request->getParam("type");
    _RspStruct rsp;
    if(type == "cpu") {
        if(s_status.cpu_profiler) {
            rsp.code = 401;
            rsp.msg = "cpu profiler running";
        } else {
            rsp.msg = "ok";
            s_status.cpu_profiler = true;
            s_status.cpu_start_time = sylar::GetCurrentMS();
            s_status.cpu_profiler_file = "/tmp/profiler-" + std::to_string(s_status.cpu_start_time) + ".prof";
            ProfilerStart(s_status.cpu_profiler_file.c_str());
        }
    } else if(type == "mem") {
        if(s_status.mem_profiler) {
            rsp.code = 401;
            rsp.msg = "mem profiler running";
        } else {
            rsp.msg = "ok";
            s_status.mem_profiler = true;
            s_status.mem_start_time = sylar::GetCurrentMS();
            s_status.mem_profiler_file = "/tmp/profiler-" + std::to_string(s_status.mem_start_time) + ".heap";
            HeapProfilerStart(s_status.mem_profiler_file.c_str());
        }
    }
    response->setBody(sylar::pack::EncodeToJsonString(rsp, 0));
    return 0;
}

int32_t handleStop(sylar::http::HttpRequest::ptr request
                     , sylar::http::HttpResponse::ptr response
                     , sylar::SocketStream::ptr session) {
    auto type = request->getParam("type");
    _RspStruct rsp;
    if(type == "cpu") {
        if(!s_status.cpu_profiler) {
            rsp.code = 401;
            rsp.msg = "cpu profiler not running";
        } else {
            rsp.msg = "ok";
            s_status.cpu_profiler = false;
            s_status.cpu_start_time = 0;
            s_status.cpu_profiler_file = "";
            ProfilerStop();
        }
    } else if(type == "mem") {
        if(!s_status.mem_profiler) {
            rsp.code = 401;
            rsp.msg = "mem profiler not running";
        } else {
            rsp.msg = "ok";
            s_status.mem_profiler = false;
            s_status.mem_start_time = 0;
            s_status.mem_profiler_file = "";
            HeapProfilerStop();
        }
    }
    response->setBody(sylar::pack::EncodeToJsonString(rsp, 0));
    return 0;
}

int32_t ProfilerServlet::handle(sylar::http::HttpRequest::ptr request
                               , sylar::http::HttpResponse::ptr response
                               , sylar::SocketStream::ptr session) {
    auto path = request->getPath();
    if(path.find("/status") != std::string::npos) {
        handleStatus(request, response, session);
    } else if(path.find("/start") != std::string::npos) {
        handleStart(request, response, session);
    } else if(path.find("/stop") != std::string::npos) {
        handleStop(request, response, session);
    } else {
        _RspStruct rsp;
        rsp.code = 404;
        rsp.msg = "invalid path";
        response->setBody(sylar::pack::EncodeToJsonString(rsp, 0));
    }
    return 0;
}

}
}
