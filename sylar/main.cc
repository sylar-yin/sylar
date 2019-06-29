#include "sylar/application.h"

int main(int argc, char** argv) {
    sylar::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}
