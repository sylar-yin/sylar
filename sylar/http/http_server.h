/**
 * @file http_server.h
 * @brief HTTP服务器封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-09
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */

#ifndef __SYLAR_HTTP_HTTP_SERVER_H__
#define __SYLAR_HTTP_HTTP_SERVER_H__

#include "sylar/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief 构造函数
     * @param[in] keepalive 是否长连接
     * @param[in] worker 工作调度器
     * @param[in] accept_worker 接收连接调度器
     */
    HttpServer(bool keepalive = false
               ,sylar::IOManager* worker = sylar::IOManager::GetThis()
               ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

    /**
     * @brief 获取ServletDispatch
     */
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}

    /**
     * @brief 设置ServletDispatch
     */
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif
