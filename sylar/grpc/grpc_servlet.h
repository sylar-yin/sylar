#ifndef __SYLAR_GRPC_SERVLET_H__
#define __SYLAR_GRPC_SERVLET_H__

#include <type_traits>

#include "sylar/http/servlet.h"
#include "grpc_stream.h"
#include "grpc_protocol.h"
#include "grpc_session.h"

#define GRPC_SERVLET_CLONE(class_name) \
    sylar::grpc::GrpcServlet::ptr clone() override { \
        return std::make_shared<class_name>(*this); \
    }
//return std::make_shared<std::remove_pointer<decltype(this)>::type>(*this);

#define GRPC_SERVLET_CTOR(class_name) \
    class_name() \
        :Base(#class_name) { \
    }

#define GRPC_SERVLET_INIT(class_name) \
    GRPC_SERVLET_CTOR(class_name) \
    GRPC_SERVLET_CLONE(class_name)

#define GRPC_SERVLET_CTOR_NAME(class_name, name) \
    class_name() \
        :Base(name) { \
    }

#define GRPC_SERVLET_INIT_NAME(class_name, name) \
    GRPC_SERVLET_CTOR_NAME(class_name, name) \
    GRPC_SERVLET_CLONE(class_name)


namespace sylar {
namespace grpc {

enum class GrpcType {
    UNARY = 0,
    SERVER = 1,
    CLIENT = 2,
    BIDIRECTION = 3    //Bidirectional
};

class GrpcServlet : public sylar::http::Servlet {
public:
    typedef std::shared_ptr<GrpcServlet> ptr;
    GrpcServlet(const std::string& name, GrpcType type);
    int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::SocketStream::ptr session) override;

    virtual int32_t process(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResponse::ptr response,
                            sylar::grpc::GrpcSession::ptr session);

    virtual int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                            sylar::grpc::GrpcResponse::ptr response,
                            sylar::grpc::GrpcStream::ptr stream,
                            sylar::grpc::GrpcSession::ptr session);

    static std::string GetGrpcPath(const std::string& ns,
                    const std::string& service, const std::string& method);

    GrpcType getType() const { return m_type;}

    virtual GrpcServlet::ptr clone() = 0;
public:
    sylar::grpc::GrpcRequest::ptr  getRequest() const { return m_request;}
    sylar::grpc::GrpcResponse::ptr getResponse() const { return m_response;}
    sylar::grpc::GrpcSession::ptr  getSession() const { return m_session;}
    sylar::grpc::GrpcStream::ptr   getStream() const { return m_stream;}
protected:
    GrpcType m_type;
    sylar::grpc::GrpcRequest::ptr m_request;
    sylar::grpc::GrpcResponse::ptr m_response;
    sylar::grpc::GrpcSession::ptr m_session;
    sylar::grpc::GrpcStream::ptr m_stream;
};

class CloneServletCreator : public sylar::http::IServletCreator {
public:
    typedef std::shared_ptr<CloneServletCreator> ptr;
    CloneServletCreator(GrpcServlet::ptr slt)
        :m_servlet(slt) {
    }

    sylar::http::Servlet::ptr get() const override {
        return m_servlet->clone();
    }

    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    GrpcServlet::ptr m_servlet;
};

class GrpcFunctionServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcFunctionServlet> ptr;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResponse::ptr response,
                                  sylar::grpc::GrpcSession::ptr session)> callback;

    typedef std::function<int32_t(sylar::grpc::GrpcRequest::ptr request,
                                  sylar::grpc::GrpcResponse::ptr response,
                                  sylar::grpc::GrpcStream::ptr stream,
                                  sylar::grpc::GrpcSession::ptr session)> stream_callback;

    GrpcFunctionServlet(GrpcType type, callback cb, stream_callback scb);

    int32_t process(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResponse::ptr response,
                    sylar::grpc::GrpcSession::ptr session) override;

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                    sylar::grpc::GrpcResponse::ptr response,
                    sylar::grpc::GrpcStream::ptr stream,
                    sylar::grpc::GrpcSession::ptr session) override;

    GrpcServlet::ptr clone() override;

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
                            sylar::grpc::GrpcResponse::ptr response,
                            sylar::grpc::GrpcSession::ptr session) {
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

//Client
template<class Req, class Rsp>
class GrpcStreamClientServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamClientServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamClientServlet Base;
    typedef sylar::grpc::GrpcServerStreamClient<Req, Rsp> ServerStream;

    GrpcStreamClientServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::CLIENT) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
        typename ServerStream::ptr reader = std::make_shared<ServerStream>(stream);
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(reader, rsp);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(typename ServerStream::ptr stream, RspPtr rsp) = 0;
};

//Server
template<class Req, class Rsp>
class GrpcStreamServerServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamServerServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamServerServlet Base;
    typedef sylar::grpc::GrpcServerStreamServer<Req, Rsp> ServerStream;

    GrpcStreamServerServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::SERVER) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
        typename ServerStream::ptr writer = std::make_shared<ServerStream>(stream);
        ReqPtr req = request->getAsPB<Req>();
        if(req) {
            int32_t rt = handle(writer, req);
            return rt;
        } else {
            //TODO log
            return -1;
        }
    }

    virtual int32_t handle(typename ServerStream::ptr writer, ReqPtr req) = 0;
};

//Bidirectional
template<class Req, class Rsp>
class GrpcStreamBidirectionServlet : public GrpcServlet {
public:
    typedef std::shared_ptr<GrpcStreamBidirectionServlet> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;
    typedef GrpcStreamBidirectionServlet Base;
    typedef sylar::grpc::GrpcServerStreamBidirection<Req, Rsp> ServerStream;

    GrpcStreamBidirectionServlet(const std::string& name)
        :GrpcServlet(name, GrpcType::BIDIRECTION) {
    }

    int32_t processStream(sylar::grpc::GrpcRequest::ptr request,
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
        typename ServerStream::ptr stm = std::make_shared<ServerStream>(stream);
        return handle(stm);
    }

    virtual int32_t handle(typename ServerStream::ptr stream) = 0;
};


#if 0
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
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
        typename Reader::ptr reader = std::make_shared<Reader>(stream);
        RspPtr rsp = std::make_shared<Rsp>();
        int32_t rt = handle(reader, rsp, request, response, stream, session);
        response->setAsPB(*rsp);
        return rt;
    }

    virtual int32_t handle(typename Reader::ptr reader, RspPtr rsp,
                           sylar::grpc::GrpcRequest::ptr request,
                           sylar::grpc::GrpcResponse::ptr response,
                           sylar::grpc::GrpcStream::ptr stream,
                           sylar::grpc::GrpcSession::ptr session) = 0;
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
                   sylar::grpc::GrpcResponse::ptr response,
                   sylar::grpc::GrpcStream::ptr stream,
                   sylar::grpc::GrpcSession::ptr session)> callback;

    GrpcStreamClientFullFunctionServlet(callback cb)
        :Base("GrpcStreamClientFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename Reader::ptr reader, RspPtr rsp,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResponse::ptr response,
                   sylar::grpc::GrpcStream::ptr stream,
                   sylar::grpc::GrpcSession::ptr session) override {
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
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
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
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
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
                           sylar::grpc::GrpcResponse::ptr response,
                           sylar::grpc::GrpcStream::ptr stream,
                           sylar::grpc::GrpcSession::ptr session) = 0;
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
                                  sylar::grpc::GrpcResponse::ptr response,
                                  sylar::grpc::GrpcStream::ptr stream,
                                  sylar::grpc::GrpcSession::ptr session)> callback;

    GrpcStreamServerFullFunctionServlet(callback cb)
        :Base("GrpcStreamServerFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(ReqPtr req, typename Writer::ptr writer,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResponse::ptr response,
                   sylar::grpc::GrpcStream::ptr stream,
                   sylar::grpc::GrpcSession::ptr session) override {
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
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
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
                          sylar::grpc::GrpcResponse::ptr response,
                          sylar::grpc::GrpcStream::ptr stream,
                          sylar::grpc::GrpcSession::ptr session) override {
        typename ReaderWriter::ptr rw = std::make_shared<ReaderWriter>(stream);
        int32_t rt = handle(rw, request, response, stream, session);
        return rt;
    }

    virtual int32_t handle(typename ReaderWriter::ptr rw,
                           sylar::grpc::GrpcRequest::ptr request,
                           sylar::grpc::GrpcResponse::ptr response,
                           sylar::grpc::GrpcStream::ptr stream,
                           sylar::grpc::GrpcSession::ptr session) = 0;
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
                                  sylar::grpc::GrpcResponse::ptr response,
                                  sylar::grpc::GrpcStream::ptr stream,
                                  sylar::grpc::GrpcSession::ptr session)> callback;

    GrpcStreamBothFullFunctionServlet(callback cb)
        :Base("GrpcStreamBothFullFunctionServlet")
        ,m_cb(cb) {
    }

    int32_t handle(typename ReaderWriter::ptr reader,
                   sylar::grpc::GrpcRequest::ptr request,
                   sylar::grpc::GrpcResponse::ptr response,
                   sylar::grpc::GrpcStream::ptr stream,
                   sylar::grpc::GrpcSession::ptr session) override {
        return m_cb(reader, request, response, stream, session);
    }

    static GrpcStreamBothFullFunctionServlet::ptr Create(callback cb) {
        return std::make_shared<GrpcStreamBothFullFunctionServlet>(cb);
    }
private:
    callback m_cb;
};

#endif


}
}

#endif
