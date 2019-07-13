#include "sylar/application.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));
    sylar::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}
