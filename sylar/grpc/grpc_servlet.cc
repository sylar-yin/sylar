#include "grpc_servlet.h"
#include "sylar/log.h"

namespace sylar {
namespace grpc {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string GrpcServlet::GetGrpcPath(const std::string& ns,
                    const std::string& service, const std::string& method) {
    std::string path = "/";
    if(!ns.empty()) {
        path += ns + ".";
    }
    path += service + "/" + method;
    return path;
}

GrpcServlet::GrpcServlet(const std::string& name, GrpcType type)
    :sylar::http::Servlet(name)
    ,m_type(type) {
}

int32_t GrpcServlet::handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) {
    std::string content_type = request->getHeader("content-type");
    if(content_type.empty()
            || content_type.find("application/grpc") == std::string::npos) {
        response->setStatus(sylar::http::HttpStatus::UNSUPPORTED_MEDIA_TYPE);
        return 1;
    }

    response->setHeader("content-type", content_type);
    auto trailer = sylar::StringUtil::Trim(request->getHeader("te"));
    if(trailer == "trailers") {
        response->setHeader("trailer", "grpc-status,grpc-message");
    }

    GrpcMessage::ptr msg = std::make_shared<GrpcMessage>();
    sylar::grpc::GrpcStreamClient::ptr cli;
    sylar::http2::Http2Session::ptr h2session = std::dynamic_pointer_cast<sylar::http2::Http2Session>(session);

    if(m_type == GrpcType::UNARY) {
        auto& body = request->getBody();
        SYLAR_LOG_INFO(g_logger) << "==== body.size=" << body.size();
        if(body.size() < 5) {
            response->setHeader("grpc-status", "100");
            response->setHeader("grpc-message", "body less 5");
            response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
            response->setReason("body less 5");
            return 1;
        }

        sylar::ByteArray::ptr ba(new sylar::ByteArray((void*)&body[0], body.size()));
        try {
            msg->compressed = ba->readFuint8();
            msg->length = ba->readFuint32();
            msg->data = ba->toString();
        } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "invalid grpc body";
            response->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
            response->setReason("invalid grpc body");
            return -1;
        }
    } else {
        auto stream_id = request->getStreamId();
        auto stream = h2session->getStream(stream_id);
        cli = std::make_shared<sylar::grpc::GrpcStreamClient>(sylar::http2::StreamClient::Create(stream));
        if(m_type == GrpcType::SERVER) {
            msg->data = *cli->recvMessageData();
            msg->length = msg->data.size();
            msg->compressed = 0;
        }
    }

    GrpcRequest::ptr greq = std::make_shared<GrpcRequest>();
    greq->setData(msg);
    greq->setRequest(request);

    GrpcResult::ptr grsp = std::make_shared<GrpcResult>(0, "", 0);
    grsp->setResponse(response);

    int rt = 0;
    if(m_type == GrpcType::UNARY) {
        rt = process(greq, grsp, h2session);
    } else {
        if(m_type != GrpcType::CLIENT) {
            SYLAR_LOG_INFO(g_logger) << "**** rsp: " << *response;
            cli->getStream()->sendResponse(grsp->getResponse(), false);
            auto& headers = response->getHeaders();
            for(auto& i : headers) {
                SYLAR_LOG_INFO(g_logger) << i.first << " - " << i.second;
            }
        }
        rt = processStream(greq, grsp, cli, h2session);
    }
    if(rt == 0) {
        if(m_type == GrpcType::UNARY || m_type == GrpcType::CLIENT) {
            response->setHeader("grpc-status", std::to_string(grsp->getResult()));
            response->setHeader("grpc-message", grsp->getError());
            auto grpc_data = grsp->getData();
            if(grpc_data) {
                std::string data;
                data.resize(grpc_data->data.size() + 5);
                sylar::ByteArray::ptr ba(new sylar::ByteArray(&data[0], data.size()));
                ba->writeFuint8(0);
                ba->writeFuint32(grpc_data->data.size());
                ba->write(grpc_data->data.c_str(), grpc_data->data.size());
                response->setBody(data);
            }
        } else {
            std::map<std::string, std::string> headers;
            headers["grpc-status"] = std::to_string(grsp->getResult());
            headers["grpc-message"] = grsp->getError();
            cli->getStream()->sendHeaders(headers, true);

            for(auto& i : headers) {
                SYLAR_LOG_INFO(g_logger) << i.first << " : " << i.second;
            }
        }
    }
    return rt;
}


GrpcFunctionServlet::GrpcFunctionServlet(GrpcType type, callback cb, stream_callback scb)
    :GrpcServlet("GrpcFunctionServlet", type)
    ,m_cb(cb)
    ,m_scb(scb) {
}

int32_t GrpcFunctionServlet::process(sylar::grpc::GrpcRequest::ptr request,
                                     sylar::grpc::GrpcResult::ptr response,
                                     sylar::http2::Http2Session::ptr session) {
    if(m_cb) {
        return m_cb(request, response, session);
    } else {
        session->sendRstStream(request->getRequest()->getStreamId(), 404);
        SYLAR_LOG_WARN(g_logger) << "GrpcFunctionServlet unhandle process " << request->getRequest()->getPath();
        return -1;
    }
}

int32_t GrpcFunctionServlet::processStream(sylar::grpc::GrpcRequest::ptr request,
                                           sylar::grpc::GrpcResult::ptr response,
                                           sylar::grpc::GrpcStreamClient::ptr stream,
                                           sylar::http2::Http2Session::ptr session) {
    if(m_scb) {
        return m_scb(request, response, stream, session);
    } else {
        session->sendRstStream(request->getRequest()->getStreamId(), 404);
        SYLAR_LOG_WARN(g_logger) << "GrpcFunctionServlet unhandle processStream " << request->getRequest()->getPath();
        return -1;
    }
}


GrpcFunctionServlet::ptr GrpcFunctionServlet::Create(GrpcType type, callback cb, stream_callback scb) {
    return std::make_shared<GrpcFunctionServlet>(type, cb, scb);
}

}
}
