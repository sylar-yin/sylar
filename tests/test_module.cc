#include "sylar/module.h"
#include "sylar/singleton.h"
#include <iostream>

class A {
public:
    A() {
        std::cout << "A::A " << this << std::endl;
    }

    ~A() {
        std::cout << "A::~A " << this << std::endl;
    }

};

class MyModule : public sylar::Module {
public:
    MyModule()
        :Module("hello", "1.0", "") {
        //sylar::Singleton<A>::GetInstance();
    }

    bool onLoad() override {
        sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onLoad------------" << std::endl;
        return true;
    }

    bool onUnload() override {
        sylar::Singleton<A>::GetInstance();
        std::cout << "-----------onUnload------------" << std::endl;
        return true;
    }
};

extern "C" {

sylar::Module* CreateModule() {
    sylar::Singleton<A>::GetInstance();
    std::cout << "=============CreateModule=================" << std::endl;
    return new MyModule;
}

void DestoryModule(sylar::Module* ptr) {
    std::cout << "=============DestoryModule=================" << std::endl;
    delete ptr;
}

}
