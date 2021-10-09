#ifndef __SYLAR_GRPC_GRPC_STREAM_H__
#define __SYLAR_GRPC_GRPC_STREAM_H__

#include <google/protobuf/message.h>
#include "sylar/http2/http2_stream.h"

namespace sylar {
namespace grpc {

class GrpcStream : public std::enable_shared_from_this<GrpcStream> {
public:
    typedef std::shared_ptr<GrpcStream> ptr;
    GrpcStream(http2::Http2Stream::ptr stream);

    int32_t sendData(const std::string& data, bool end_stream = false);
    http2::DataFrame::ptr recvData();

    int32_t sendMessage(const google::protobuf::Message& msg, bool end_stream = false);
    std::shared_ptr<std::string> recvMessageData();

    template<class T>
    std::shared_ptr<T> recvMessage() {
        auto d = recvMessageData();
        if(!d) {
            return nullptr;
        }
        try {
            std::shared_ptr<T> data = std::make_shared<T>();
            if(data->ParseFromString(*d)) {
                return data;
            }
        } catch(...) {
        }
        return nullptr;
    }

    http2::Http2Stream::ptr getStream() const { return m_stream;}

    bool getEnableGzip() const { return m_enableGzip;}
    void setEnableGzip(bool v) { m_enableGzip =v;}
private:
    http2::Http2Stream::ptr m_stream;
    bool m_enableGzip = false;
};

class GrpcServerStream {
public:
    typedef std::shared_ptr<GrpcServerStream> ptr;
    GrpcServerStream(GrpcStream::ptr stream);

    GrpcStream::ptr getStream() const { return m_stream;}
protected:
    GrpcStream::ptr m_stream;
};

//GrpcType::CLIENT
template<class Req, class Rsp>
class GrpcServerStreamClient : public GrpcServerStream {
public:
    typedef std::shared_ptr<GrpcServerStreamClient> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcServerStreamClient(GrpcStream::ptr stream)
        :GrpcServerStream(stream) {
    }

    ReqPtr recv() {
        return m_stream->recvMessage<Req>();
    }
};

//GrpcType::SERVER
template<class Req, class Rsp>
class GrpcServerStreamServer : public GrpcServerStream {
public:
    typedef std::shared_ptr<GrpcServerStreamServer> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcServerStreamServer(GrpcStream::ptr stream)
        :GrpcServerStream(stream) {
    }

    int32_t send(RspPtr msg) {
        return m_stream->sendMessage(*msg);
    }
};

//GrpcType::BIDI
template<class Req, class Rsp>
class GrpcServerStreamBidirection : public GrpcServerStream {
public:
    typedef std::shared_ptr<GrpcServerStreamBidirection> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcServerStreamBidirection(GrpcStream::ptr stream)
        :GrpcServerStream(stream) {
    }

    int32_t send(RspPtr msg) {
        return m_stream->sendMessage(*msg);
    }

    ReqPtr recv() {
        return m_stream->recvMessage<Req>();
    }
};


class GrpcClientStream {
public:
    typedef std::shared_ptr<GrpcClientStream> ptr;
    GrpcClientStream(GrpcStream::ptr stream);

    GrpcStream::ptr getStream() const { return m_stream;}
protected:
    GrpcStream::ptr m_stream;
};

//GrpcType::CLIENT
template<class Req, class Rsp>
class GrpcClientStreamClient : public GrpcClientStream {
public:
    typedef std::shared_ptr<GrpcClientStreamClient> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcClientStreamClient(GrpcStream::ptr stream)
        :GrpcClientStream(stream) {
    }

    int32_t send(ReqPtr req) {
        return m_stream->sendMessage(*req);
    }

    RspPtr closeAndRecv() {
        m_stream->sendData("", true);
        return m_stream->recvMessage<Rsp>();
    }
};

//GrpcType::SERVER
template<class Req, class Rsp>
class GrpcClientStreamServer : public GrpcClientStream {
public:
    typedef std::shared_ptr<GrpcClientStreamServer> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcClientStreamServer(GrpcStream::ptr stream)
        :GrpcClientStream(stream) {
    }

    RspPtr recv() {
        return m_stream->recvMessage<Rsp>();
    }
};

//GrpcType::BIDI
template<class Req, class Rsp>
class GrpcClientStreamBidirection : public GrpcClientStream {
public:
    typedef std::shared_ptr<GrpcClientStreamBidirection> ptr;
    typedef std::shared_ptr<Req> ReqPtr;
    typedef std::shared_ptr<Rsp> RspPtr;

    GrpcClientStreamBidirection(GrpcStream::ptr stream)
        :GrpcClientStream(stream) {
    }

    RspPtr recv() {
        return m_stream->recvMessage<Rsp>();
    }

    int32_t send(ReqPtr req) {
        return m_stream->sendMessage(*req);
    }

    int32_t close() {
        return m_stream->sendData("", true);
    }
};

/*
class GrpcStreamBase {
public:
    typedef std::shared_ptr<GrpcStreamBase> ptr;
    GrpcStreamBase() {};
    GrpcStreamBase(GrpcStreamClient::ptr client);
    GrpcStreamClient::ptr getClient() const { return m_client;}
    int32_t close(int err = 0);
protected:
    GrpcStreamClient::ptr m_client;
};

template<class T>
class GrpcStreamReader : virtual public GrpcStreamBase {
public:
    typedef std::shared_ptr<GrpcStreamReader> ptr;
    GrpcStreamReader() {}
    GrpcStreamReader(GrpcStreamClient::ptr client)
        :GrpcStreamBase(client) {
    }

    std::shared_ptr<T> recvMessage() {
        return m_client->recvMessage<T>();
    }
};

template<class T>
class GrpcStreamWriter : virtual public GrpcStreamBase {
public:
    typedef std::shared_ptr<GrpcStreamWriter> ptr;
    GrpcStreamWriter();
    GrpcStreamWriter(GrpcStreamClient::ptr client)
        :GrpcStreamBase(client) {
    }

    int32_t sendMessage(std::shared_ptr<T> msg) {
        return m_client->sendMessage(*msg, false);
    }
};

template<class R, class W>
class GrpcStream : public GrpcStreamBase {
public:
    typedef std::shared_ptr<GrpcStream> ptr;
    GrpcStream(GrpcStreamClient::ptr client)
        :GrpcStreamBase(client) {
    }

    std::shared_ptr<R> recvMessage() {
        return m_client->recvMessage<R>();
    }

    int32_t sendMessage(std::shared_ptr<W> msg) {
        return m_client->sendMessage(*msg, false);
    }
};
template<class Req, class Rsp> using GrpcStreamSession = GrpcStream<Req, Rsp>;
template<class Req, class Rsp> using GrpcStreamConnection = GrpcStream<Rsp, Req>;
*/

}
}

#endif
