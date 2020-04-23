// Copyright 2018 Sen Han <00hnes@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ACO_H
#define ACO_H

#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#ifdef ACO_USE_VALGRIND
    #include <valgrind/valgrind.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ACO_VERSION_MAJOR 1
#define ACO_VERSION_MINOR 2
#define ACO_VERSION_PATCH 4

#ifdef __i386__
    #define ACO_REG_IDX_RETADDR 0
    #define ACO_REG_IDX_SP 1
    #define ACO_REG_IDX_BP 2
    #define ACO_REG_IDX_FPU 6
#elif __x86_64__
    #define ACO_REG_IDX_RETADDR 4
    #define ACO_REG_IDX_SP 5
    #define ACO_REG_IDX_BP 7
    #define ACO_REG_IDX_FPU 8
#else
    #error "platform no support yet"
#endif

typedef struct {
    void*  ptr;
    size_t sz;
    size_t valid_sz;
    // max copy size in bytes
    size_t max_cpsz;
    // copy from share stack to this save stack
    size_t ct_save;
    // copy from this save stack to share stack 
    size_t ct_restore;
} aco_save_stack_t;

struct aco_s;
typedef struct aco_s aco_t;

typedef struct {
    void*  ptr;            
    size_t sz;
    void*  align_highptr;
    void*  align_retptr;
    size_t align_validsz;
    size_t align_limit;
    aco_t* owner;

    char guard_page_enabled;
    void* real_ptr;
    size_t real_sz;

#ifdef ACO_USE_VALGRIND
    unsigned long valgrind_stk_id;
#endif
} aco_share_stack_t;

typedef void (*aco_cofuncp_t)(void);

struct aco_s{
    // cpu registers' state
#ifdef __i386__
    #ifdef ACO_CONFIG_SHARE_FPU_MXCSR_ENV
        void*  reg[6];
    #else
        void*  reg[8];
    #endif
#elif __x86_64__
    #ifdef ACO_CONFIG_SHARE_FPU_MXCSR_ENV
        void*  reg[8];
    #else
        void*  reg[9];
    #endif
#else
    #error "platform no support yet"
#endif
    aco_t* main_co;
    void*  arg;
    char   is_end;

    aco_cofuncp_t fp;
    
    aco_save_stack_t  save_stack;
    aco_share_stack_t* share_stack;
};

#define aco_likely(x) (__builtin_expect(!!(x), 1))

#define aco_unlikely(x) (__builtin_expect(!!(x), 0))

#define aco_assert(EX)  ((aco_likely(EX))?((void)0):(abort()))

#define aco_assertptr(ptr)  ((aco_likely((ptr) != NULL))?((void)0):(abort()))

#define aco_assertalloc_bool(b)  do {  \
    if(aco_unlikely(!(b))){    \
        fprintf(stderr, "Aborting: failed to allocate memory: %s:%d:%s\n", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
        abort();    \
    }   \
} while(0)

#define aco_assertalloc_ptr(ptr)  do {  \
    if(aco_unlikely((ptr) == NULL)){    \
        fprintf(stderr, "Aborting: failed to allocate memory: %s:%d:%s\n", \
            __FILE__, __LINE__, __PRETTY_FUNCTION__);    \
        abort();    \
    }   \
} while(0)

#if defined(aco_attr_no_asan)
    #error "aco_attr_no_asan already defined"
#endif
#if defined(ACO_USE_ASAN)
    #if defined(__has_feature)
        #if __has_feature(__address_sanitizer__)
            #define aco_attr_no_asan \
                __attribute__((__no_sanitize_address__))
        #endif
    #endif
    #if defined(__SANITIZE_ADDRESS__) && !defined(aco_attr_no_asan)
        #define aco_attr_no_asan \
            __attribute__((__no_sanitize_address__))
    #endif
#endif
#ifndef aco_attr_no_asan
    #define aco_attr_no_asan
#endif

extern void aco_runtime_test(void);

extern void aco_thread_init(aco_cofuncp_t last_word_co_fp);

extern void* acosw(aco_t* from_co, aco_t* to_co) __asm__("acosw"); // asm

extern void aco_save_fpucw_mxcsr(void* p) __asm__("aco_save_fpucw_mxcsr");  // asm

extern void aco_funcp_protector_asm(void) __asm__("aco_funcp_protector_asm"); // asm

extern void aco_funcp_protector(void);

extern aco_share_stack_t* aco_share_stack_new(size_t sz);

extern void aco_share_stack_init(aco_share_stack_t* ass, void* stack, size_t sz);

aco_share_stack_t* aco_share_stack_new2(size_t sz, char guard_page_enabled);

extern void aco_share_stack_destroy(aco_share_stack_t* sstk);

extern aco_t* aco_create(
        aco_t* main_co,
        aco_share_stack_t* share_stack, 
        size_t save_stack_sz, 
        aco_cofuncp_t fp, void* arg
    );

// aco's Global Thread Local Storage variable `co`
extern __thread aco_t* aco_gtls_co;

aco_attr_no_asan
extern void aco_resume(aco_t* resume_co);

//extern void aco_yield1(aco_t* yield_co);
#define aco_yield1(yield_co) do {             \
    aco_assertptr((yield_co));                    \
    aco_assertptr((yield_co)->main_co);           \
    acosw((yield_co), (yield_co)->main_co);   \
} while(0)

#define aco_yield() do {        \
    aco_yield1(aco_gtls_co);    \
} while(0)

#define aco_get_arg() (aco_gtls_co->arg)

#define aco_get_co() ({(void)0; aco_gtls_co;})

#define aco_co() ({(void)0; aco_gtls_co;})

extern void aco_destroy(aco_t* co);

#define aco_is_main_co(co) ({((co)->main_co) == NULL;})

#define aco_exit1(co) do {     \
    (co)->is_end = 1;           \
    aco_assert((co)->share_stack->owner == (co)); \
    (co)->share_stack->owner = NULL; \
    (co)->share_stack->align_validsz = 0; \
    aco_yield1((co));            \
    aco_assert(0);                  \
} while(0)

#define aco_exit() do {       \
    aco_exit1(aco_gtls_co); \
} while(0)

#ifdef __cplusplus
}
#endif

#endif
