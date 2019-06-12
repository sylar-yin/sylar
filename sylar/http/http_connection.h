/**
 * @file http_connection.h
 * @brief HTTP客户端类
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-11
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include "sylar/socket_stream.h"
#include "http.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP客户端类
 */
class HttpConnection : public SocketStream {
public:
    /// HTTP客户端类智能指针
    typedef std::shared_ptr<HttpConnection> ptr;

    /**
     * @brief 构造函数
     * @param[in] sock Socket类
     * @param[in] owner 是否掌握所有权
     */
    HttpConnection(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP响应
     */
    HttpResponse::ptr recvResponse();

    /**
     * @brief 发送HTTP请求
     * @param[in] req HTTP请求结构
     */
    int sendRequest(HttpRequest::ptr req);
};

}
}

#endif
