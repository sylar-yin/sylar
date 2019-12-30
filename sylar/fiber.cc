#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>
#include <sys/mman.h>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

class MMapStackAllocator {
public:
    static void* Alloc(size_t size) {
        return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    }

    static void Dealloc(void* vp, size_t size) {
        munmap(vp, size);
    }
};

using StackAllocator = MallocStackAllocator;
//using StackAllocator = MMapStackAllocator;

//class FiberPool {
//public:
//    FiberPool(size_t max_size)
//        :m_maxSize(max_size) {
//    }
//    ~FiberPool() {
//        for(auto& i : m_mems) {
//            free(i);
//        }
//    }
//    void* alloc(size_t size);
//    void dealloc(void* ptr);
//private:
//    struct MemItem {
//        size_t size;
//        char data[];
//    };
//private:
//    size_t m_maxSize;
//    std::list<MemItem*> m_mems;
//};
//
//void* FiberPool::alloc(size_t size) {
//    for(auto it = m_mems.begin();
//            it != m_mems.end(); ++it) {
//        if((*it)->size >= size) {
//            auto p = (*it)->data;
//            m_mems.erase(it);
//            return p;
//        }
//    }
//    MemItem* mi = (MemItem*)malloc(sizeof(size_t) + size);
//    mi->size = size;
//    return mi->data;
//}
//
//void FiberPool::dealloc(void* ptr) {
//    MemItem* mi = (MemItem*)((char*)ptr - sizeof(size_t));
//    m_mems.push_front(mi);
//    if(m_mems.size() > m_maxSize) {
//        MemItem* v = m_mems.back();
//        m_mems.pop_back();
//        free(v);
//        SYLAR_LOG_INFO(g_logger) << "poll full";
//    }
//}
//
//static thread_local FiberPool s_fiber_pool(1024);

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}


Fiber* NewFiber() {
    //Fiber* p = (Fiber*)malloc(sizeof(Fiber));
    //return new (p) Fiber();
    //p->Fiber();
    //return p;
    return new Fiber();
}

Fiber* NewFiber(std::function<void()> cb, size_t stacksize, bool use_caller) {
    stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    //Fiber* p = (Fiber*)malloc(sizeof(Fiber) + stacksize);
    //Fiber* p = (Fiber*)s_fiber_pool.alloc(sizeof(Fiber) + stacksize);
    Fiber* p = (Fiber*)StackAllocator::Alloc(sizeof(Fiber) + stacksize);
    return new (p) Fiber(cb, stacksize, use_caller);
    //p->Fiber(cb, stacksize, use_caller);
    //return p;
}

void FreeFiber(Fiber* ptr) {
    ptr->~Fiber();
    //free(ptr);
    //s_fiber_pool.dealloc(ptr);
    //StackAllocator::Dealloc(ptr, ptr->m_stacksize + sizeof(Fiber));
    StackAllocator::Dealloc(ptr, 0);
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_init(&m_ctx);
#endif

    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;
    //m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
    m_stacksize = stacksize;

    //m_stack = StackAllocator::Alloc(m_stacksize);
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    if(!use_caller) {
        m_ctx = make_fcontext((char*)m_stack + m_stacksize, m_stacksize, &Fiber::MainFunc);
    } else {
        m_ctx = make_fcontext((char*)m_stack + m_stacksize, m_stacksize, &Fiber::CallerMainFunc);
    }
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    m_ctx.ss_size = m_stacksize;
    m_ctx.ss_sp = (char*)m_stack;
    if(!use_caller) {
        coctx_make(&m_ctx, &Fiber::MainFunc, 0, 0);
    } else {
        coctx_make(&m_ctx, &Fiber::CallerMainFunc, 0, 0);
    }
#endif

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        SYLAR_ASSERT(m_state == TERM
                || m_state == EXCEPT
                || m_state == INIT);

        //StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
}

//重置协程函数，并重置状态
//INIT，TERM, EXCEPT
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM
            || m_state == EXCEPT
            || m_state == INIT);
    m_cb = cb;

#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    m_ctx = make_fcontext((char*)m_stack + m_stacksize, m_stacksize, &Fiber::MainFunc);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_make(&m_ctx, &Fiber::MainFunc, 0, 0);
#endif
    m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    jump_fcontext(&t_threadFiber->m_ctx, m_ctx, 0);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_swap(&t_threadFiber->m_ctx, &m_ctx);
#endif
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    jump_fcontext(&m_ctx, t_threadFiber->m_ctx, 0);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_swap(&m_ctx, &t_threadFiber->m_ctx);
#endif
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    jump_fcontext(&Scheduler::GetMainFiber()->m_ctx, m_ctx, 0);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_swap(&Scheduler::GetMainFiber()->m_ctx, &m_ctx);
#endif
}

//切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
    jump_fcontext(&m_ctx, Scheduler::GetMainFiber()->m_ctx, 0);
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
    coctx_swap(&m_ctx, &Scheduler::GetMainFiber()->m_ctx);
#endif
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(NewFiber());
    //Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

//协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    //cur->m_state = HOLD;
    cur->swapOut();
}

//总协程数
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}
#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
void Fiber::MainFunc() {
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
void Fiber::MainFunc(intptr_t vp) {
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
void* Fiber::MainFunc(void*, void*) {
#endif
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

#if FIBER_CONTEXT_TYPE == FIBER_UCONTEXT
void Fiber::CallerMainFunc() {
#elif FIBER_CONTEXT_TYPE == FIBER_FCONTEXT
void Fiber::CallerMainFunc(intptr_t vp) {
#elif FIBER_CONTEXT_TYPE == FIBER_LIBCO
void* Fiber::CallerMainFunc(void*, void*) {
#endif
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));

}

}
