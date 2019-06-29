/**
 * @file singleton.h
 * @brief 单例模式封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-18
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar {

namespace {

template<class T, class X, int N>
T& GetInstanceX() {
    static T v;
    return v;
}

template<class T, class X, int N>
std::shared_ptr<T> GetInstancePtr() {
    static std::shared_ptr<T> v(new T);
    return v;
}


}

/**
 * @brief 单例模式封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class Singleton {
public:
    /**
     * @brief 返回单例裸指针
     */
    static T* GetInstance() {
        static T v;
        return &v;
        //return &GetInstanceX<T, X, N>();
    }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    /**
     * @brief 返回单例智能指针
     */
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
        //return GetInstancePtr<T, X, N>();
    }
};

}

#endif
