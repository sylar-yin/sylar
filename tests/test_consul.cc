#include "sylar/sylar.h"
#include "sylar/consul_client.h"
#include "sylar/pack/json_encoder.h"

void run() {
    sylar::ConsulClient::ptr client = std::make_shared<sylar::ConsulClient>();
    client->setUrl("http://consul-sit.mid.io:8500");

    sylar::ConsulRegisterInfo::ptr info = std::make_shared<sylar::ConsulRegisterInfo>();
    info->id = sylar::GetConsulUniqID(8080);
    info->name = "test-xxx";
    info->tags.push_back("aaa");
    info->address = "127.0.0.1";
    //info->address = "10.195.3.127";
    info->meta["m1"] = "m2";
    info->port = 8080;
    info->check = std::make_shared<sylar::ConsulCheck>();
    info->check->interval = "10s";
    info->check->http = "http://127.0.0.1:8080/health/check";
    //info->check->http = "http://10.195.3.127:8080/actuator/health";
    info->check->deregisterCriticalServiceAfter = "10s";

    
    std::cout << sylar::pack::EncodeToJsonString(info, 0) << std::endl;
    std::cout << "register: " << client->serviceRegister(info) << std::endl;

    std::map<std::string, std::list<sylar::ConsulServiceInfo::ptr> > m;
    if(client->serviceQuery({"test-xxx", "recommend-guard"}, m)) {
        std::cout << sylar::pack::EncodeToJsonString(m, 0) << std::endl;
    } else {
        std::cout << "serviceQuery fail" << std::endl;
    }

    sleep(1);
    std::cout << "unregister: " << client->serviceUnregister(info->id) << std::endl;
    
}

int main(int argc, char** argv) {
    sylar::IOManager io(2);
    io.schedule(run);
    io.addTimer(10000, [](){}, true);
    io.stop();
    return 0;
}
