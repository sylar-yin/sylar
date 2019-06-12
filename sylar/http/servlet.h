/**
 * @file servlet.h
 * @brief Servlet封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-08
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "sylar/thread.h"

namespace sylar {
namespace http {

/**
 * @brief Servlet封装
 */
class Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Servlet> ptr;

    /**
     * @brief 构造函数
     * @param[in] name 名称
     */
    Servlet(const std::string& name)
        :m_name(name) {}

    /**
     * @brief 析构函数
     */
    virtual ~Servlet() {}

    /**
     * @brief 处理请求
     * @param[in] request HTTP请求
     * @param[in] response HTTP响应
     * @param[in] session HTTP连接
     * @return 是否处理成功
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) = 0;
                   
    /**
     * @brief 返回Servlet名称
     */
    const std::string& getName() const { return m_name;}
protected:
    /// 名称
    std::string m_name;
};

/**
 * @brief 函数式Servlet
 */
class FunctionServlet : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<FunctionServlet> ptr;
    /// 函数回调类型定义
    typedef std::function<int32_t (sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session)> callback;


    /**
     * @brief 构造函数
     * @param[in] cb 回调函数
     */
    FunctionServlet(callback cb);
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;
private:
    /// 回调函数
    callback m_cb;
};

/**
 * @brief Servlet分发器
 */
class ServletDispatch : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<ServletDispatch> ptr;
    /// 读写锁类型定义
    typedef RWMutex RWMutexType;

    /**
     * @brief 构造函数
     */
    ServletDispatch();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;

    /**
     * @brief 添加servlet
     * @param[in] uri uri
     * @param[in] slt serlvet
     */
    void addServlet(const std::string& uri, Servlet::ptr slt);

    /**
     * @brief 添加servlet
     * @param[in] uri uri
     * @param[in] cb FunctionServlet回调函数
     */
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /sylar_*
     * @param[in] slt servlet
     */
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);

    /**
     * @brief 添加模糊匹配servlet
     * @param[in] uri uri 模糊匹配 /sylar_*
     * @param[in] cb FunctionServlet回调函数
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 删除servlet
     * @param[in] uri uri
     */
    void delServlet(const std::string& uri);

    /**
     * @brief 删除模糊匹配servlet
     * @param[in] uri uri
     */
    void delGlobServlet(const std::string& uri);

    /**
     * @brief 返回默认servlet
     */
    Servlet::ptr getDefault() const { return m_default;}

    /**
     * @brief 设置默认servlet
     * @param[in] v servlet
     */
    void setDefault(Servlet::ptr v) { m_default = v;}


    /**
     * @brief 通过uri获取servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    Servlet::ptr getServlet(const std::string& uri);

    /**
     * @brief 通过uri获取模糊匹配servlet
     * @param[in] uri uri
     * @return 返回对应的servlet
     */
    Servlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取servlet
     * @param[in] uri uri
     * @return 优先精准匹配,其次模糊匹配,最后返回默认
     */
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    /// 读写互斥量
    RWMutexType m_mutex;
    /// 精准匹配servlet MAP
    /// uri(/sylar/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    /// 模糊匹配servlet 数组
    /// uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    /// 默认servlet，所有路径都没匹配到时使用
    Servlet::ptr m_default;
};

/**
 * @brief NotFoundServlet(默认返回404)
 */
class NotFoundServlet : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<NotFoundServlet> ptr;
    /**
     * @brief 构造函数
     */
    NotFoundServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;

};

}
}

#endif
