#include "sylar/module.h"

namespace name_space {

class MyModule : public sylar::Module {
public:
    typedef std::shared_ptr<MyModule> ptr;
    MyModule();
    bool onLoad() override;
    bool onUnload() override;
    bool onServerReady() override;
    bool onServerUp() override;
};

}
