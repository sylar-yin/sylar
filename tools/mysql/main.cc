#include <iostream>
#include "schema.pb.h"
#include "sylar/sylar.h"
#include "util.h"
#include "sylar/util/pb_dynamic_message.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv) {
    sylar::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");

    bool is_print_help = false;
    if(!sylar::EnvMgr::GetInstance()->init(argc, argv)) {
        is_print_help = true;
    }

    if(sylar::EnvMgr::GetInstance()->has("p")) {
        is_print_help = true;
    }

    if(is_print_help) {
        sylar::EnvMgr::GetInstance()->printHelp();
        return 1;
    }

    std::string conf_path = sylar::EnvMgr::GetInstance()->getConfigPath();
    SYLAR_LOG_INFO(g_logger) << "conf_path: " << conf_path;
    sylar::Config::LoadFromConfDir(conf_path, true);

    auto tb = utils::InitTableInfo("test", "test", "tests");
    SYLAR_LOG_INFO(g_logger) << "tb=" << tb;
    if(tb) {
        SYLAR_LOG_INFO(g_logger) << sylar::PBToJsonString(*tb)
            << " - " << tb->comment()
            << " - " << sylar::ProtoToJson(*tb);
    }

    return 0;
}
