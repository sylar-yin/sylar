/**
 * @file thread.h
 * @brief 线程相关的封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-31
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include "mutex.h"

namespace sylar {

/**
 * @brief 线程类
 */
class Thread : Noncopyable {
public:
    /// 线程智能指针类型
    typedef std::shared_ptr<Thread> ptr;

    /**
     * @brief 构造函数
     * @param[in] cb 线程执行函数
     * @param[in] name 线程名称
     */
    Thread(std::function<void()> cb, const std::string& name);

    /**
     * @brief 析构函数
     */
    ~Thread();

    /**
     * @brief 线程ID
     */
    pid_t getId() const { return m_id;}

    /**
     * @brief 线程名称
     */
    const std::string& getName() const { return m_name;}

    /**
     * @brief 等待线程执行完成
     */
    void join();

    /**
     * @brief 获取当前的线程指针
     */
    static Thread* GetThis();

    /**
     * @brief 获取当前的线程名称
     */
    static const std::string& GetName();

    /**
     * @brief 设置当前线程名称
     * @param[in] name 线程名称
     */
    static void SetName(const std::string& name);
private:

    /**
     * @brief 线程执行函数
     */
    static void* run(void* arg);
private:
    /// 线程id
    pid_t m_id = -1;
    /// 线程结构
    pthread_t m_thread = 0;
    /// 线程执行函数
    std::function<void()> m_cb;
    /// 线程名称
    std::string m_name;
    /// 信号量
    Semaphore m_semaphore;
};

}

#endif
