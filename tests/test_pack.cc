#include "sylar/sylar.h"
#include "sylar/pack/pack.h"
#include "sylar/pack/json_decoder.h"
#include "sylar/pack/json_encoder.h"
#include "sylar/pack/yaml_decoder.h"
#include "sylar/pack/yaml_encoder.h"
#include "sylar/pack/rapidjson_encoder.h"
#include "sylar/pack/rapidjson_decoder.h"
#include "sylar/pack/bytearray_encoder.h"
#include "sylar/pack/bytearray_decoder.h"
#include <gperftools/profiler.h>
#include <gperftools/heap-profiler.h>

struct Person {
    int id;
    std::string name;
    bool sex;
    double money;

    std::vector<int> arr_int = {1, 2, 3};

    SYLAR_PACK(A("_id", id, "_sex", sex), O(name, money, arr_int));
};

struct PersonOut {
    int id;
    std::string name;
    int sex;
    double money;
};

struct Men : public Person {
    int value = 0;
    PersonOut po;
    SYLAR_PACK(I(Person), O(value, po));

    bool operator==(const Men& oth) const {
        return value == oth.value
            && id == oth.id
            && sex == oth.sex
            && name == oth.name
            && money == oth.money;
    }
};

SYLAR_PACK_OUT(PersonOut, A("_id", id, "_sex", sex), O(name, money));

void test() {
    Person p;
    p.id = 101;
    p.sex = true;
    p.money = 10.01;
    p.name = "sylar";

    sylar::pack::JsonEncoder je;
    je.encode(p, 0);

    std::cout << je.getValue() << std::endl;

    Person p2;
    sylar::pack::JsonDecoder jd(je.getValue());
    jd.decode(p2, 0);

    std::cout << p2.id << " - " << p2.name << " - " << p2.sex << " - " << p2.money << std::endl;
}

void testOut() {
    PersonOut p;
    p.id = 101;
    p.sex = true;
    p.money = 10.01;
    p.name = "sylar";

    sylar::pack::JsonEncoder je;
    je.encode(p, 0);

    std::cout << je.getValue() << std::endl;

    PersonOut p2;
    sylar::pack::JsonDecoder jd(je.getValue());
    jd.decode(p2, 0);

    std::cout << p2.id << " - " << p2.name << " - " << p2.sex << " - " << p2.money << std::endl;
}

void testMan() {
    Men p;
    p.id = 101;
    p.sex = true;
    p.money = 10.01;
    p.name = "sylar";
    p.value = 110;

    sylar::pack::JsonEncoder je;
    je.encode(p, 0);

    std::cout << je.getValue() << std::endl;

    Men p2;
    sylar::pack::JsonDecoder jd(je.getValue());
    jd.decode(p2, 0);

    std::cout << p2.id << " - " << p2.name << " - " << p2.sex << " - " << p2.money << " - " << p2.value << std::endl;
}

void test_array() {
    std::vector<Men> vs;

    Men a;
    a.id = 10;
    a.name = "men";
    a.value = 101;

    vs.push_back(a);
    vs.push_back(a);

    sylar::pack::JsonEncoder je;
    je.encode(vs, 0);

    std::cout << je.getValue() << std::endl;

    sylar::pack::JsonDecoder jd(je.getValue());
    std::vector<Men> vs2;
    jd.decode(vs2, 0);

    std::cout << (vs == vs2) << std::endl;
    std::cout << sylar::pack::EncodeToJsonString(vs, 0) << std::endl;
    std::cout << sylar::pack::EncodeToJsonString(vs2, 0) << std::endl;
    std::cout << sylar::pack::EncodeToJsonString("你好", 0) << std::endl;
}

std::string JsonToString(const Json::Value & root)
{
	static Json::Value def = []() {
		Json::Value def;
		Json::StreamWriterBuilder::setDefaults(&def);
		def["emitUTF8"] = true;
		return def;
	}();

	std::ostringstream stream;
	Json::StreamWriterBuilder stream_builder;
	stream_builder.settings_ = def;//Config emitUTF8
	std::unique_ptr<Json::StreamWriter> writer(stream_builder.newStreamWriter());
	writer->write(root, &stream);
	return stream.str();
}

void test_yaml() {
    std::vector<Men> vs;

    Men a;
    a.id = 10;
    a.name = "men";
    a.value = 101;

    vs.push_back(a);
    a.id = 11;
    a.name = "women";
    a.value = 201;
    vs.push_back(a);

    std::cout << "test_yaml" << std::endl;
    auto str = sylar::pack::EncodeToYamlString(vs, 0);
    std::cout << str << std::endl;
    std::cout << "test_yaml" << std::endl;

    std::vector<Men> vs2;
    sylar::pack::DecodeFromYamlString(str, vs2, 0);
    std::cout << sylar::pack::EncodeToJsonString(vs2, 0) << std::endl;
    std::cout << sylar::pack::EncodeToJsonString(vs, 0) << std::endl;

    //YAML::Node node;
    //node.push_back(1);
    //node.push_back(2);
    //node["arr"].push_back(3);
    //node["arr"].push_back(4);
    //std::cout << node << std::endl;
    std::vector<int> arr {1,2,3, 1};
    std::cout << sylar::pack::EncodeToYamlString(arr, 0) << std::endl;
    //YAML::Node node;
    //node = 10;
    //std::cout << node << std::endl;
}

void test_map() {
    std::map<std::string, int> m, m2;
    m["id"] = 1001;
    m["sex"] = 20;

    auto str = sylar::pack::EncodeToJsonString(m, 0);
    sylar::pack::DecodeFromJsonString(str, m2, 0);

    std::cout << "test_map: " << (m == m2) << std::endl;

    std::cout << "test_map: " << str << std::endl;
    std::cout << "test_map: " << sylar::pack::EncodeToJsonString(m2, 0) << std::endl;
}

void test_map2() {
    std::map<std::string, int> m, m2;
    m["id"] = 1001;
    m["sex"] = 20;

    auto str = sylar::pack::EncodeToYamlString(m, 0);
    sylar::pack::DecodeFromYamlString(str, m2, 0);

    std::cout << "test_map: " << (m == m2) << std::endl;

    std::cout << "test_map: " << str << std::endl;
    std::cout << "test_map: " << sylar::pack::EncodeToYamlString(m2, 0) << std::endl;
}

void test_rapid() {
    std::vector<Men> m, m2;
    m.resize(2);
    m[0].id = 101;
    m[1].id = 102;
    m[0].name = "你好";
    m[1].name = "世界";
    //std::map<std::string, std::string> m;
    //m["id"] = "\"";
    //m["age"] = "[";
    //m["name"] = "你好";
    std::string str = sylar::pack::EncodeToRapidJsonString(m, 0);
    std::cout << "test_rapid: " << str << std::endl;
    sylar::pack::DecodeFromRapidJsonString(str, m2, 0);
    std::cout << "test_rapid: " << sylar::pack::EncodeToRapidJsonString(m2, 0) << std::endl;
}

void test_bytearray() {
    std::vector<Men> m, m2;
    m.resize(2);
    m[0].id = 101;
    m[1].id = 102;
    m[0].name = "你好";
    m[1].name = "世界";
    //std::map<std::string, std::string> m;
    //m["id"] = "\"";
    //m["age"] = "[";
    //m["name"] = "你好";
    auto ba = sylar::pack::EncodeToByteArray(m, 0);
    ba->setPosition(0);
    std::cout << "test_bytearray: " << sylar::pack::EncodeToRapidJsonString(m, 0) << std::endl;
    sylar::pack::DecodeFromByteArray(ba, m2, 0);
    std::cout << "test_bytearray: " << sylar::pack::EncodeToRapidJsonString(m2, 0) << std::endl;
}

void test_shared() {
    std::shared_ptr<Men> m = std::make_shared<Men>();
    std::shared_ptr<Men> m2;
    m->id = 101;
    m->name = "哈哈";

    std::string str = sylar::pack::EncodeToRapidJsonString(m, 0);
    std::cout << "test_shared_rapid: " << str << std::endl;

    sylar::pack::DecodeFromRapidJsonString(str, m2, 0);
    std::cout << "test_shared_rapid: " << sylar::pack::EncodeToRapidJsonString(m2, 0) << std::endl;
}

int main(int argc, char** argv) {
    //std::cout << SYLAR_STRING(SYLAR_PACK_OUT(A, A("id", id, "_name", name), O(sex,age))) << std::endl;
    //std::cout << SYLAR_STRING(SYLAR_PACK(A("id", id, "_name", name), O(sex,age))) << std::endl;
    //ProfilerStart("test.prof");
    HeapProfilerStart("test.heap");
    test();
    std::cout << sizeof(long double) << std::endl;
    std::cout << sizeof(long long) << std::endl;
    testOut();
    testMan();
    test_array();
    Json::Value value;
    value["name"] = "你好";
    std::cout << JsonToString(value) << std::endl;
    test_yaml();
    YAML::Node ynode;
    auto yyn = ynode["id"];
    yyn = 10;
    ynode["name"] = "sylar";
    std::cout << ynode << std::endl;
    //Person p;
    //std::cout << sylar::pack::EncodeToYamlString(p, 0) << std::endl;
    test_map();
    test_map2();
    test_rapid();
    test_shared();
    test_bytearray();
    //ProfilerStop();
    HeapProfilerStop();
    return 0;
}
