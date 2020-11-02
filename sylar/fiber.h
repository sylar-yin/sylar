/**
 * @file fiber.h
 * @brief 协程封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-24
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>

#define FIBER_UCONTEXT      1
#define FIBER_FCONTEXT      2
#define FIBER_LIBCO         3 
#define FIBER_LIBACO        4

#ifndef FIBER_CONTEXT_TYPE
#define FIBER_CONTEXT_TYPE  FIBER_FCONTEXT
#endif

#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
#include <ucontext.h>
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
#include "sylar/fcontext/fcontext.h"
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
#include "sylar/libco/coctx.h"
#elif FIBER_CONTEXT_TYPE == FIBER_LIBACO
#include "sylar/libaco/aco.h"
#endif

namespace sylar {

class Scheduler;
class Fiber;

Fiber* NewFiber();
Fiber* NewFiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
void FreeFiber(Fiber* ptr);

/**
 * @brief 协程类
 */
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
friend Fiber* NewFiber();
friend Fiber* NewFiber(std::function<void()> cb, size_t stacksize, bool use_caller);
friend void FreeFiber(Fiber* ptr);
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     */
    enum State {
        /// 初始化状态
        INIT,
        /// 暂停状态
        HOLD,
        /// 执行中状态
        EXEC,
        /// 结束状态
        TERM,
        /// 可执行状态
        READY,
        /// 异常状态
        EXCEPT
    };
private:
    /**
     * @brief 无参构造函数
     * @attention 每个线程第一个协程的构造
     */
    Fiber();

    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
public:

    /**
     * @brief 析构函数
     */
    ~Fiber();

    /**
     * @brief 重置协程执行函数,并设置状态
     * @pre getState() 为 INIT, TERM, EXCEPT
     * @post getState() = INIT
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程切换到运行状态
     * @pre getState() != EXEC
     * @post getState() = EXEC
     */
    void swapIn();

    /**
     * @brief 将当前协程切换到后台
     */
    void swapOut();

    /**
     * @brief 将当前线程切换到执行状态
     * @pre 执行的为当前线程的主协程
     */
    void call();

    /**
     * @brief 将当前线程切换到后台
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     */
    void back();

    /**
     * @brief 返回协程id
     */
    uint64_t getId() const { return m_id;}

    /**
     * @brief 返回协程状态
     */
    State getState() const { return m_state;}
public:

    /**
     * @brief 设置当前线程的运行协程
     * @param[in] f 运行协程
     */
    static void SetThis(Fiber* f);

    /**
     * @brief 返回当前所在的协程
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 将当前协程切换到后台,并设置为READY状态
     * @post getState() = READY
     */
    static void YieldToReady();

    /**
     * @brief 将当前协程切换到后台,并设置为HOLD状态
     * @post getState() = HOLD
     */
    static void YieldToHold();

    /**
     * @brief 返回当前协程的总数量
     */
    static uint64_t TotalFibers();

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程主协程
     */
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT || FIBER_CONTEXT_TYPE == FIBER_LIBACO
    static void MainFunc();
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    static void MainFunc(intptr_t vp);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    static void* MainFunc(void*, void*);
#endif

    /**
     * @brief 协程执行函数
     * @post 执行完成返回到线程调度协程
     */
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT || FIBER_CONTEXT_TYPE == FIBER_LIBACO
    static void CallerMainFunc();
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    static void CallerMainFunc(intptr_t vp);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    static void* CallerMainFunc(void*, void*);
#endif

    /**
     * @brief 获取当前协程的id
     */
    static uint64_t GetFiberId();
private:
    /// 协程id
    uint64_t m_id = 0;
    /// 协程运行栈大小
    uint32_t m_stacksize = 0;
    /// 协程状态
    State m_state = INIT;
    /// 协程运行函数
    std::function<void()> m_cb;
    /// 协程上下文
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    ucontext_t m_ctx;
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    fcontext_t m_ctx = nullptr;
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_t m_ctx;
#elif FIBER_CONTEXT_TYPE == FIBER_LIBACO
    aco_t* m_ctx = nullptr;
    aco_share_stack_t m_astack;
#endif
    /// 协程运行栈指针
    char m_stack[];
};

}

#endif
