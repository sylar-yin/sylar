/**
 * @file http_parser.h
 * @brief HTTP协议解析封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-11
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */

#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

/**
 * @brief HTTP请求解析类
 */
class HttpRequestParser {
public:
    /// HTTP解析类的智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;

    /**
     * @brief 构造函数
     */
    HttpRequestParser();

    /**
     * @brief 解析协议
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @return 返回实际解析的长度,并且将已解析的数据移除
     */
    size_t execute(char* data, size_t len);

    /**
     * @brief 是否解析完成
     * @return 是否解析完成
     */
    int isFinished();

    /**
     * @brief 是否有错误
     * @return 是否有错误
     */
    int hasError(); 

    /**
     * @brief 返回HttpRequest结构体
     */
    HttpRequest::ptr getData() const { return m_data;}

    /**
     * @brief 设置错误
     * @param[in] v 错误值
     */
    void setError(int v) { m_error = v;}

    /**
     * @brief 获取消息体长度
     */
    uint64_t getContentLength();

    /**
     * @brief 获取http_parser结构体
     */
    const http_parser& getParser() const { return m_parser;}
public:
    /**
     * @brief 返回HttpRequest协议解析的缓存大小
     */
    static uint64_t GetHttpRequestBufferSize();

    /**
     * @brief 返回HttpRequest协议的最大消息体大小
     */
    static uint64_t GetHttpRequestMaxBodySize();
private:
    /// http_parser
    http_parser m_parser;
    /// HttpRequest结构
    HttpRequest::ptr m_data;
    /// 错误码
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

/**
 * @brief Http响应解析结构体
 */
class HttpResponseParser {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /**
     * @brief 构造函数
     */
    HttpResponseParser();

    /**
     * @brief 解析HTTP响应协议
     * @param[in, out] data 协议数据内存
     * @param[in] len 协议数据内存大小
     * @param[in] chunck 是否在解析chunck
     * @return 返回实际解析的长度,并且移除已解析的数据
     */
    size_t execute(char* data, size_t len, bool chunck);

    /**
     * @brief 是否解析完成
     */
    int isFinished();

    /**
     * @brief 是否有错误
     */
    int hasError(); 

    /**
     * @brief 返回HttpResponse
     */
    HttpResponse::ptr getData() const { return m_data;}

    /**
     * @brief 设置错误码
     * @param[in] v 错误码
     */
    void setError(int v) { m_error = v;}

    /**
     * @brief 获取消息体长度
     */
    uint64_t getContentLength();

    /**
     * @brief 返回httpclient_parser
     */
    const httpclient_parser& getParser() const { return m_parser;}
public:
    /**
     * @brief 返回HTTP响应解析缓存大小
     */
    static uint64_t GetHttpResponseBufferSize();

    /**
     * @brief 返回HTTP响应最大消息体大小
     */
    static uint64_t GetHttpResponseMaxBodySize();
private:
    /// httpclient_parser
    httpclient_parser m_parser;
    /// HttpResponse
    HttpResponse::ptr m_data;
    /// 错误码
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

}
}

#endif
