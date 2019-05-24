#include "sylar/sylar.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv) {
    sylar::Scheduler sc;
    sc.start();
    sc.stop();
    return 0;
}
