#ifndef __SYLAR_GRPC_SERVLET_H__
#define __SYLAR_GRPC_SERVLET_H__

#include "sylar/http/servlet.h"
#include "sylar/grpc/grpc_stream.h"

namespace sylar {
namespace grpc {

enum class GrpcType {
    UNARY = 0,
    SERVER = 1,
    CLIENT = 2,
    BOTH = 3
};

class GrpcServlet : public sylar::http::Servlet {
public:
    typedef std::shared_ptr<GrpcServlet> ptr;
    GrpcServlet(const std::string& name, GrpcType type);
    int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::http2::Http2Session::ptr session);

    virtual int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::grpc::GrpcStreamClient::ptr stream,
                            sylar::http2::Http2Session::ptr session);

    static std::string GetGrpcPath(const std::string& ns,
                    const std::string& service, const std::string& method);

protected:
    GrpcType m_type;
};

class GrpcFunctionServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcFunctionServlet> ptr;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::http2::Http2Session::ptr session)> callback;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::grpc::GrpcStreamClient::ptr stream,
                                  sylar::http2::Http2Session::ptr session)> stream_callback;

    GrpcFunctionServlet(GrpcType type, callback cb, stream_callback scb);

    int32_t process(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResult::ptr response,
                    sylar::http2::Http2Session::ptr session) override;

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResult::ptr response,
                    sylar::grpc::GrpcStreamClient::ptr stream,
                    sylar::http2::Http2Session::ptr session) override;

    static GrpcFunctionServlet::ptr Create(GrpcType type, callback cb, stream_callback scb);
private:
    callback m_cb;
    stream_callback m_scb;
};

template<class Req, class Rsp>
class GrpcUnaryServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcUnaryServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcUnaryServlet Base;

    GrpcUnaryServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::UNARY) {
    }

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::http2::Http2Session::ptr session) {
        auto req = request->getAsPB<Req>();
        if(!req) {
            response->setResult(100);
            response->setError("invalid request pb");
            return -1;
        }
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(req, rsp);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(ReqPtr req, RspPtr rsp) = 0;
};

template<class Req, class Rsp>
class GrpcUnaryFullServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcUnaryFullServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcUnaryFullServlet Base;

    GrpcUnaryFullServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::UNARY) {
    }

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResult::ptr response,
                            sylar::http2::Http2Session::ptr session) {
        auto req = request->getAsPB<Req>();
        if(!req) {
            response->setResult(100);
            response->setError("invalid request pb");
            return -1;
        }
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(req, rsp, request, response, session);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(typename Base::ReqPtr req, typename Base::RspPtr rsp
                           ,sylar::grpc::GrpcRequest::ptr request
                           ,sylar::grpc::GrpcResult::ptr response
                           ,sylar::http2::Http2Session::ptr session) = 0;
};

template<class Req, class Rsp>
class GrpcUnaryFunctionServlet : public GrpcUnaryServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcUnaryFunctionServlet> ptr;
    typedef typename GrpcUnaryServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcUnaryServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcUnaryServlet<Req, Rsp>::Base Base;

    typedef std::function<int32_t(ReqPtr req, RspPtr rsp)> callback;

    GrpcUnaryFunctionServlet(callback cb)
        :Base("GrpcUnaryFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(ReqPtr req, RspPtr rsp) override {
        return m_cb(req, rsp);
    }

    static GrpcUnaryFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcUnaryFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

template<class Req, class Rsp>
class GrpcUnaryFullFunctionServlet : public GrpcUnaryFullServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcUnaryFullFunctionServlet> ptr;
    typedef typename GrpcUnaryFullServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcUnaryFullServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcUnaryFullServlet<Req, Rsp>::Base Base;

    typedef std::function<int32_t(ReqPtr req, RspPtr rsp,
                                  sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::http2::Http2Session::ptr session)> callback;

    GrpcUnaryFullFunctionServlet(callback cb)
        :Base("GrpcUnaryFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(ReqPtr req, RspPtr rsp,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResult::ptr response,
                   sylar::http2::Http2Session::ptr session) override {
        return m_cb(req, rsp, request, response, session);
    }

    static GrpcUnaryFullFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcUnaryFullFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

//Client
template<class Req, class Rsp>
class GrpcStreamClientServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamClientServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamClientServlet Base;
    typedef sylar::grpc::GrpcStreamReader<Req> Reader;

    GrpcStreamClientServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::CLIENT) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        typename Reader::ptr reader = std::make_shared<Reader>(stream);
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(reader, rsp);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(typename Reader::ptr reader, RspPtr rsp) = 0;
};

template<class Req, class Rsp>
class GrpcStreamClientFunctionServlet : public GrpcStreamClientServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamClientFunctionServlet> ptr;
    typedef typename GrpcStreamClientServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamClientServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamClientServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamClientServlet<Req, Rsp>::Reader Reader;


    typedef std::function<int32_t(typename Reader::ptr reader, RspPtr rsp)> callback;

    GrpcStreamClientFunctionServlet(callback cb)
        :Base("GrpcStreamClientFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename Reader::ptr reader, RspPtr rsp) override {
        return m_cb(reader, rsp);
    }

    static GrpcStreamClientFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamClientFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

template<class Req, class Rsp>
class GrpcStreamClientFullServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamClientFullServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamClientFullServlet Base;
    typedef sylar::grpc::GrpcStreamReader<Req> Reader;

    GrpcStreamClientFullServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::CLIENT) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        typename Reader::ptr reader = std::make_shared<Reader>(stream);
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(reader, rsp, request, response, stream, session);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(typename Reader::ptr reader, RspPtr rsp,
                           sylar::grpc::GrpcRequest::ptr request,
                           sylar::grpc::GrpcResult::ptr response,
                           sylar::grpc::GrpcStreamClient::ptr stream,
                           sylar::http2::Http2Session::ptr session) = 0;
};

template<class Req, class Rsp>
class GrpcStreamClientFullFunctionServlet : public GrpcStreamClientFullServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamClientFullFunctionServlet> ptr;
    typedef typename GrpcStreamClientFullServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamClientFullServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamClientFullServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamClientFullServlet<Req, Rsp>::Reader Reader;


    typedef std::function<int32_t(typename Reader::ptr reader, RspPtr rsp,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResult::ptr response,
                   sylar::grpc::GrpcStreamClient::ptr stream,
                   sylar::http2::Http2Session::ptr session)> callback;

    GrpcStreamClientFullFunctionServlet(callback cb)
        :Base("GrpcStreamClientFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename Reader::ptr reader, RspPtr rsp,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResult::ptr response,
                   sylar::grpc::GrpcStreamClient::ptr stream,
                   sylar::http2::Http2Session::ptr session) override {
        return m_cb(reader, rsp, request, response, stream, session);
    }

    static GrpcStreamClientFullFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamClientFullFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

//Server
template<class Req, class Rsp>
class GrpcStreamServerServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamServerServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamServerServlet Base;
    typedef sylar::grpc::GrpcStreamWriter<Rsp> Writer;

    GrpcStreamServerServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::SERVER) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        auto req = request->getAsPB<Req>();
        if(!req) {
            response->setResult(100);
            response->setError("invalid request pb");
            return -1;
        }
        typename Writer::ptr writer = std::make_shared<Writer>(stream);
        int32_t rt = handle(req, writer);
        return rt;
    }

    virtual int32_t handle(ReqPtr req, typename Writer::ptr writer) = 0;
};

template<class Req, class Rsp>
class GrpcStreamServerFunctionServlet : public GrpcStreamServerServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamServerFunctionServlet> ptr;
    typedef typename GrpcStreamServerServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamServerServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamServerServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamServerServlet<Req, Rsp>::Writer Writer;


    typedef std::function<int32_t(ReqPtr req, typename Writer::ptr writer)> callback;

    GrpcStreamServerFunctionServlet(callback cb)
        :Base("GrpcStreamServerFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(ReqPtr req, typename Writer::ptr writer) override {
        return m_cb(req, writer);
    }

    static GrpcStreamServerFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamServerFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

template<class Req, class Rsp>
class GrpcStreamServerFullServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamServerFullServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamServerFullServlet Base;
    typedef sylar::grpc::GrpcStreamWriter<Rsp> Writer;

    GrpcStreamServerFullServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::SERVER) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        auto req = request->getAsPB<Req>();
        if(!req) {
            response->setResult(100);
            response->setError("invalid request pb");
            return -1;
        }
        typename Writer::ptr writer = std::make_shared<Writer>(stream);
        int32_t rt = handle(req, writer, request, response, stream, session);
        return rt;
    }

    virtual int32_t handle(ReqPtr req, typename Writer::ptr writer,
                           sylar::grpc::GrpcRequest::ptr request,
                           sylar::grpc::GrpcResult::ptr response,
                           sylar::grpc::GrpcStreamClient::ptr stream,
                           sylar::http2::Http2Session::ptr session) = 0;
};

template<class Req, class Rsp>
class GrpcStreamServerFullFunctionServlet : public GrpcStreamServerFullServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamServerFullFunctionServlet> ptr;
    typedef typename GrpcStreamServerFullServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamServerFullServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamServerFullServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamServerFullServlet<Req, Rsp>::Writer Writer;


    typedef std::function<int32_t(ReqPtr req, typename Writer::ptr writer,
                                  sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::grpc::GrpcStreamClient::ptr stream,
                                  sylar::http2::Http2Session::ptr session)> callback;

    GrpcStreamServerFullFunctionServlet(callback cb)
        :Base("GrpcStreamServerFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(ReqPtr req, typename Writer::ptr writer,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResult::ptr response,
                   sylar::grpc::GrpcStreamClient::ptr stream,
                   sylar::http2::Http2Session::ptr session) override {
        return m_cb(req, writer, request, response, stream, session);
    }

    static GrpcStreamServerFullFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamServerFullFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

//BOTH
template<class Req, class Rsp>
class GrpcStreamBothServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamBothServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamBothServlet Base;
    typedef sylar::grpc::GrpcStreamSession<Req, Rsp> ReaderWriter;

    GrpcStreamBothServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::BOTH) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        typename ReaderWriter::ptr rw = std::make_shared<ReaderWriter>(stream);
        int32_t rt = handle(rw);
        return rt;
    }

    virtual int32_t handle(typename ReaderWriter::ptr rw) = 0;
};

template<class Req, class Rsp>
class GrpcStreamBothFunctionServlet : public GrpcStreamBothServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamBothFunctionServlet> ptr;
    typedef typename GrpcStreamBothServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamBothServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamBothServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamBothServlet<Req, Rsp>::ReaderWriter ReaderWriter;


    typedef std::function<int32_t(typename ReaderWriter::ptr reader)> callback;

    GrpcStreamBothFunctionServlet(callback cb)
        :Base("GrpcStreamBothFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename ReaderWriter::ptr reader) override {
        return m_cb(reader);
    }

    static GrpcStreamBothFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamBothFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

template<class Req, class Rsp>
class GrpcStreamBothFullServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamBothFullServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamBothFullServlet Base;
    typedef sylar::grpc::GrpcStreamSession<Req, Rsp> ReaderWriter;

    GrpcStreamBothFullServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::BOTH) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResult::ptr response,
                          sylar::grpc::GrpcStreamClient::ptr stream,
                          sylar::http2::Http2Session::ptr session) override {
        typename ReaderWriter::ptr rw = std::make_shared<ReaderWriter>(stream);
        int32_t rt = handle(rw, request, response, stream, session);
        return rt;
    }

    virtual int32_t handle(typename ReaderWriter::ptr rw,
                           sylar::grpc::GrpcRequest::ptr request,
                           sylar::grpc::GrpcResult::ptr response,
                           sylar::grpc::GrpcStreamClient::ptr stream,
                           sylar::http2::Http2Session::ptr session) = 0;
};

template<class Req, class Rsp>
class GrpcStreamBothFullFunctionServlet : public GrpcStreamBothFullServlet<Req, Rsp> {
public:
    typedef std::shared_ptr<GrpcStreamBothFullFunctionServlet> ptr;
    typedef typename GrpcStreamBothFullServlet<Req, Rsp>::ReqPtr ReqPtr;
    typedef typename GrpcStreamBothFullServlet<Req, Rsp>::RspPtr RspPtr;
    typedef typename GrpcStreamBothFullServlet<Req, Rsp>::Base Base;
    typedef typename GrpcStreamBothFullServlet<Req, Rsp>::ReaderWriter ReaderWriter;


    typedef std::function<int32_t(typename ReaderWriter::ptr reader,
                                  sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResult::ptr response,
                                  sylar::grpc::GrpcStreamClient::ptr stream,
                                  sylar::http2::Http2Session::ptr session)> callback;

    GrpcStreamBothFullFunctionServlet(callback cb)
        :Base("GrpcStreamBothFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename ReaderWriter::ptr reader,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResult::ptr response,
                   sylar::grpc::GrpcStreamClient::ptr stream,
                   sylar::http2::Http2Session::ptr session) override {
        return m_cb(reader, request, response, stream, session);
    }

    static GrpcStreamBothFullFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamBothFullFunctionServlet>(cb);
    }
private:
    callback m_cb;
};


}
}

#endif
