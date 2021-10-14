#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/compiler/importer.h>
#include "sylar/sylar.h"
#include "sylar/util/pb_dynamic_message.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

using google::protobuf::util::JsonStringToMessage;
using google::protobuf::util::MessageToJsonString;
std::string PbToString(const google::protobuf::Message& msg) {
    std::string out;
    google::protobuf::util::JsonPrintOptions options;
    //options.add_whitespace = true;
    //options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    MessageToJsonString(msg, &out, options);
    return out;
}

void TestEmptyMessage(const google::protobuf::DescriptorPool& pool) {
    auto type = pool.FindMessageTypeByName("EmptyMessage");
    google::protobuf::DynamicMessageFactory factory(&pool);

    std::shared_ptr<google::protobuf::Message> msg(factory.GetPrototype(type)->New());

    auto reflect = msg->GetReflection();
    auto field = type->FindFieldByName("int32_filed");

    reflect->SetInt32(msg.get(), field, 1024);
    reflect->MutableUnknownFields(msg.get())->AddFixed32(2, 10);

    SYLAR_LOG_INFO(g_logger) << msg->DebugString();
    SYLAR_LOG_INFO(g_logger) << sylar::PBToJsonString(*msg);
    SYLAR_LOG_INFO(g_logger) << PbToString(*msg);
}

void TestEmptyMessage2(const google::protobuf::DescriptorPool& poolxx) {
    auto& pool = *google::protobuf::DescriptorPool::generated_pool();
    auto type = pool.FindMessageTypeByName("EmptyMessage");
    google::protobuf::DynamicMessageFactory factory(&pool);

    std::shared_ptr<google::protobuf::Message> msg(factory.GetPrototype(type)->New());

    auto reflect = msg->GetReflection();
    auto field = type->FindFieldByName("int32_filed");

    reflect->SetInt32(msg.get(), field, 1024);
    reflect->MutableUnknownFields(msg.get())->AddFixed32(2, 10);

    SYLAR_LOG_INFO(g_logger) << msg->DebugString();
    SYLAR_LOG_INFO(g_logger) << sylar::PBToJsonString(*msg);
    SYLAR_LOG_INFO(g_logger) << PbToString(*msg);
}

void TestFileDescriptorSet() {
    int fd = open("tests/test.proto", O_RDONLY);
    google::protobuf::FileDescriptorSet fds;
    SYLAR_LOG_INFO(g_logger) << "Parse FileDescriptorSet " << fds.ParseFromFileDescriptor(fd);
    SYLAR_LOG_INFO(g_logger) << fds.DebugString() << " - " << fds.file_size();
    SYLAR_LOG_INFO(g_logger) << sylar::PBToJsonString(fds);
}

void testPool() {
    auto st = new google::protobuf::compiler::DiskSourceTree;
    auto stdb = new google::protobuf::compiler::SourceTreeDescriptorDatabase(st);
    google::protobuf::DescriptorPool* pool = new google::protobuf::DescriptorPool(stdb);
    //(void)pool;

    st->MapPath("", "");
    st->MapPath("", "tests");

    //google::protobuf::compiler::Importer importer(st, nullptr);
    //auto pb = importer.Import("tests/test.proto");
    //SYLAR_LOG_INFO(g_logger) << "==== pb=" << pb << (pb ? pb->DebugString() : "");
    //sleep(20);
    //pb = importer.Import("tests/test.proto");
    //SYLAR_LOG_INFO(g_logger) << "==== pb=" << pb << (pb ? pb->DebugString() : "");

    //pool->Import("tests/test.proto");
    //auto pb = importer.pool()->FindFileByName("tests/test.proto");
    std::vector<std::string> files = {
        "test.proto",
        "test2.proto",
        //"tests/test.proto",
    };
    for(auto& i : files) {
        auto pb = pool->FindFileByName(i);
        std::cout << i << " - " << pb << std::endl;
        //delete pb;
    }
    //auto pb = pool->FindFileByName("/root/workspace/sylar/tests/test.proto");
    SYLAR_LOG_INFO(g_logger) << "msg=" << pool->FindMessageTypeByName("test.HelloRequest");
    SYLAR_LOG_INFO(g_logger) << "msg=" << pool->FindMessageTypeByName("test2.Test2");
}

void testSource() {
    google::protobuf::compiler::DiskSourceTree sourceTree;
    sourceTree.MapPath("", "tests");
    google::protobuf::compiler::Importer importer(&sourceTree, NULL);
    importer.Import("test.proto");
    SYLAR_LOG_INFO(g_logger) << "pb=" << importer.pool()->FindMessageTypeByName("test.HelloRequest");
}

void test_pb_dynamic_message() {
    //sylar::util::PbDynamicMessageFactoryMgr::GetInstance()->
    std::shared_ptr<google::protobuf::Message> msg;
    std::shared_ptr<google::protobuf::Message> msg2;
    {
        sylar::PbDynamicMessageFactory::ptr item = std::make_shared<sylar::PbDynamicMessageFactory>();

        item->addProtoPathMap("", "");
        //item->addProtoPathMap("", "tests");

        //std::vector<std::string> files = {
        //    "test.proto",
        //    "test2.proto",
        //};
        //for(auto& i : files) {
        //    auto pb = item->findFileByName(i);
        //    std::cout << i << " - " << pb << std::endl;
        //}


        auto rt = item->loadDir("tests");
        for(auto i : rt) {
            SYLAR_LOG_INFO(g_logger) << "loaded: " << i->name();
        }
        msg = item->createMessageByName("test2.Test2");
        msg2 = item->createMessageByName("test2.Test2");
    }

        sylar::PbDynamicMessage::ptr dm = std::make_shared<sylar::PbDynamicMessage>(msg);
        dm->setInt32Value("id", 100);
        dm->setStringValue("name", "sylar");
        auto sub = dm->mutableMessage("request");
        sub->setStringValue("id", "sylar-id");
        sub->addUnknowFieldString(2, "sylar-msg");

        auto fs = dm->listFields();
        for(auto& i : fs) {
            SYLAR_LOG_INFO(g_logger) << "---" << i->name();
        }

    SYLAR_LOG_INFO(g_logger) << "msg=" << msg << " - " << msg->DebugString()
        << " - " << sylar::PBToJsonString(*msg) << " - "
        << dm->getDataSize();

    std::string tmp;
    msg->SerializeToString(&tmp);
    msg2->ParseFromString(tmp);

    SYLAR_LOG_INFO(g_logger) << "msg2=" << msg2 << " - " << msg2->DebugString()
        << " - " << sylar::PBToJsonString(*msg2);
}

void test_pb_dynamic_proto() {
    auto item = sylar::PbDynamicMessageFactory::CreateWithUnderlay();
    auto fs = item->loadDir("tests");
    for(auto i : fs) {
        SYLAR_LOG_INFO(g_logger) << "***" << i->DebugString();
    }

    auto proto = std::make_shared<sylar::PbDynamicMessageProto>();
    proto->setName("DPMessage");
    proto->setPackage("sylar");
    proto->addInt64Field(1, "id");
    proto->addStringField(2, "name");
    proto->addDoubleField(3, "double_arr", sylar::PbDynamicMessageProto::REPEATED);
    proto->addMessageField(4, "message", "test2.Test2");

    auto f = item->addPbDynamicProto(proto, "", {"test2.proto"});
    std::cout << f->DebugString() << std::endl;

    //f = item->addPbDynamicProto(proto, "", {"test2.proto"});
    //std::cout << f->DebugString() << std::endl;

    auto m = item->createDynamicMessageByName("DPMessage");
    m->setInt64Value("id", 1024);
    m->setStringValue("name", "sylar-yin");
    m->addDoubleValue("double_arr", 3.14);
    m->addDoubleValue("double_arr", 4.68);

    auto mm = m->mutableMessage("message");
    mm->setInt32Value("id", 4096);

    auto de = item->createDynamicEmptyMessage();
    de->addUnknowFieldString(1, "hello_request_id");
    de->addUnknowFieldString(2, "hello_request_msg");
    mm->addUnknowFieldMessage(3, *de->getData());

    auto d = m->getData();
    SYLAR_LOG_INFO(g_logger) << "==== " << d->DebugString()
        << " - " << sylar::PBToJsonString(*d)
        << " - " << PbToString(*d);

    SYLAR_LOG_INFO(g_logger) << sylar::ToSnakeString("ABCTestInfo");
    SYLAR_LOG_INFO(g_logger) << sylar::ToCamelString("abc_test_info");
    SYLAR_LOG_INFO(g_logger) << sylar::ToCamelString("abc_test_info", false);
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "hello dynamic message";
    google::protobuf::DescriptorPool pool;
    google::protobuf::FileDescriptorProto file;
    file.set_name("empty_message.proto");
    auto m = file.add_message_type();
    m->set_name("EmptyMessage");

    auto f = m->add_field();
    f->set_name("int32_filed");
    f->set_number(1);
    f->set_type(google::protobuf::FieldDescriptorProto::TYPE_INT32);

    SYLAR_LOG_INFO(g_logger) << "BuildFile: " << pool.BuildFile(file);
    TestEmptyMessage(pool);

    TestFileDescriptorSet();
    SYLAR_LOG_INFO(g_logger) << file.DebugString();

    testPool();
    //testSource();
    //test_pb_dynamic_message();
    //test_pb_dynamic_proto();
    return 0;
}
