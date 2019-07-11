#include "sylar/email/email.h"
#include "sylar/email/smtp.h"

void test() {
    sylar::EMail::ptr email = sylar::EMail::Create(
            "user@163.com", "passwd",
            "hello world", "<B>hi xxx</B>hell world", {"564628276@qq.com"});
    sylar::EMailEntity::ptr entity = sylar::EMailEntity::CreateAttach("sylar/sylar.h");
    if(entity) {
        email->addEntity(entity);
    }

    entity = sylar::EMailEntity::CreateAttach("sylar/address.cc");
    if(entity) {
        email->addEntity(entity);
    }

    auto client = sylar::SmtpClient::Create("smtp.163.com", 465, true);
    if(!client) {
        std::cout << "connect smtp.163.com:25 fail" << std::endl;
        return;
    }

    auto result = client->send(email, true);
    std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    std::cout << client->getDebugInfo() << std::endl;
    //result = client->send(email, true);
    //std::cout << "result=" << result->result << " msg=" << result->msg << std::endl;
    //std::cout << client->getDebugInfo() << std::endl;
}

int main(int argc, char** argv) {
    sylar::IOManager iom(1);
    iom.schedule(test);
    iom.stop();
    return 0;
}
