/**
 * @file uri.h
 * @brief URI封装类
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-13
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace sylar {

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/

/**
 * @brief URI类
 */
class Uri {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Uri> ptr;

    /**
     * @brief 创建Uri对象
     * @param uri uri字符串
     * @return 解析成功返回Uri对象否则返回nullptr
     */
    static Uri::ptr Create(const std::string& uri);

    /**
     * @brief 构造函数
     */
    Uri();

    /**
     * @brief 返回scheme
     */
    const std::string& getScheme() const { return m_scheme;}

    /**
     * @brief 返回用户信息
     */
    const std::string& getUserinfo() const { return m_userinfo;}

    /**
     * @brief 返回host
     */
    const std::string& getHost() const { return m_host;}

    /**
     * @brief 返回路径
     */
    const std::string& getPath() const;

    /**
     * @brief 返回查询条件
     */
    const std::string& getQuery() const { return m_query;}

    /**
     * @brief 返回fragment
     */
    const std::string& getFragment() const { return m_fragment;}

    /**
     * @brief 返回端口
     */
    int32_t getPort() const;

    /**
     * @brief 设置scheme
     * @param v scheme
     */
    void setScheme(const std::string& v) { m_scheme = v;}

    /**
     * @brief 设置用户信息
     * @param v 用户信息
     */
    void setUserinfo(const std::string& v) { m_userinfo = v;}

    /**
     * @brief 设置host信息
     * @param v host
     */
    void setHost(const std::string& v) { m_host = v;}

    /**
     * @brief 设置路径
     * @param v 路径
     */
    void setPath(const std::string& v) { m_path = v;}

    /**
     * @brief 设置查询条件
     * @param v
     */
    void setQuery(const std::string& v) { m_query = v;}

    /**
     * @brief 设置fragment
     * @param v fragment
     */
    void setFragment(const std::string& v) { m_fragment = v;}

    /**
     * @brief 设置端口号
     * @param v 端口
     */
    void setPort(int32_t v) { m_port = v;}

    /**
     * @brief 序列化到输出流
     * @param os 输出流
     * @return 输出流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string toString() const;

    /**
     * @brief 获取Address
     */
    Address::ptr createAddress() const;
private:

    /**
     * @brief 是否默认端口
     */
    bool isDefaultPort() const;
private:
    /// schema
    std::string m_scheme;
    /// 用户信息
    std::string m_userinfo;
    /// host
    std::string m_host;
    /// 路径
    std::string m_path;
    /// 查询参数
    std::string m_query;
    /// fragment
    std::string m_fragment;
    /// 端口
    int32_t m_port;
};

}

#endif
