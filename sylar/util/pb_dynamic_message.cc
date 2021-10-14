#include "pb_dynamic_message.h"
#include "sylar/util.h"
#include "sylar/log.h"
#include <google/protobuf/util/json_util.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string ProtoToJson(const google::protobuf::Message& m) {
    std::string out;
    google::protobuf::util::JsonPrintOptions options;
    //options.add_whitespace = true;
    //options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(m, &out, options);
    return out;
}

bool JsonToProto(const std::string& json, google::protobuf::Message& m) {
    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;
    return google::protobuf::util::JsonStringToMessage(json, &m, options).ok();
}

PbDynamicMessage::PbDynamicMessage(std::shared_ptr<google::protobuf::Message> data)
    :m_data(data) {
    m_reflection = m_data->GetReflection();
    m_descriptor = m_data->GetDescriptor();
}

const google::protobuf::FieldDescriptor* PbDynamicMessage::getFieldByName(const std::string& name) const {
    return m_descriptor->FindFieldByName(name);
}

bool PbDynamicMessage::hasFieldValue(const std::string& name) const {
    auto fd = getFieldByName(name);
    if(fd) {
        return hasFieldValue(fd);
    }
    return false;
}

bool PbDynamicMessage::hasFieldValue(const google::protobuf::FieldDescriptor* field) const {
    return m_reflection->HasField(*m_data, field);
}

int32_t PbDynamicMessage::getFieldValueSize(const std::string& name) const {
    auto fd = getFieldByName(name);
    if(fd) {
        return getFieldValueSize(fd);
    }
    return 0;
}

int32_t PbDynamicMessage::getFieldValueSize(const google::protobuf::FieldDescriptor* field) const {
    return m_reflection->FieldSize(*m_data, field);
}


void PbDynamicMessage::clearFieldValue(const std::string& name) {
    auto fd = getFieldByName(name);
    if(fd) {
        clearFieldValue(fd);
    }
}

void PbDynamicMessage::clearFieldValue(const google::protobuf::FieldDescriptor* field) {
    m_reflection->ClearField(m_data.get(), field);
}

std::vector<const google::protobuf::FieldDescriptor*> PbDynamicMessage::listFields() {
    std::vector<const google::protobuf::FieldDescriptor*> rt;
    m_reflection->ListFields(*m_data, &rt);
    return rt;
}

#define GEN_GET_SET(funcname, pbtype, type) \
type PbDynamicMessage::get##funcname(const std::string& name) const {  \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return get##funcname(fd); \
    } \
    return 0; \
} \
type PbDynamicMessage::get##funcname(const google::protobuf::FieldDescriptor* field) const { \
    return m_reflection->Get##pbtype(*m_data, field); \
} \
void PbDynamicMessage::set##funcname(const std::string& name, type v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        set##funcname(fd, v); \
    } \
} \
void PbDynamicMessage::set##funcname(const google::protobuf::FieldDescriptor* field, type v) { \
    m_reflection->Set##pbtype(m_data.get(), field, v); \
}

GEN_GET_SET(Int32Value, Int32, int32_t);
GEN_GET_SET(Uint32Value, UInt32, uint32_t);
GEN_GET_SET(Int64Value, Int64, int64_t);
GEN_GET_SET(Uint64Value, UInt64, uint64_t);
GEN_GET_SET(FloatValue, Float, float);
GEN_GET_SET(DoubleValue, Double, double);
GEN_GET_SET(BoolValue, Bool, bool);

#undef GEN_GET_SET

#define GEN_GET_SET(funcname, pbtype, type) \
type PbDynamicMessage::get##funcname(const std::string& name) const {  \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return get##funcname(fd); \
    } \
    return 0; \
} \
type PbDynamicMessage::get##funcname(const google::protobuf::FieldDescriptor* field) const { \
    return m_reflection->Get##pbtype(*m_data, field); \
} \
void PbDynamicMessage::set##funcname(const std::string& name, const type& v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        set##funcname(fd, v); \
    } \
} \
void PbDynamicMessage::set##funcname(const google::protobuf::FieldDescriptor* field, const type& v) { \
    m_reflection->Set##pbtype(m_data.get(), field, v); \
}
GEN_GET_SET(StringValue, String, std::string);
#undef GEN_GET_SET

#define GEN_GEt_SEt_ADD(funcname, pbtype, type) \
type PbDynamicMessage::getRepeated##funcname(const std::string& name, int32_t idx) const { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return getRepeated##funcname(fd, idx); \
    } \
    return 0; \
} \
type PbDynamicMessage::getRepeated##funcname(const google::protobuf::FieldDescriptor* field, int32_t idx) const { \
    return m_reflection->GetRepeated##pbtype(*m_data, field, idx); \
} \
void    PbDynamicMessage::setRepeated##funcname(const std::string& name, int32_t idx, type v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return setRepeated##funcname(fd, idx, v); \
    } \
} \
void    PbDynamicMessage::setRepeated##funcname(const google::protobuf::FieldDescriptor* field, int32_t idx, type v) { \
    m_reflection->SetRepeated##pbtype(m_data.get(), field, idx, v); \
} \
void    PbDynamicMessage::add##funcname(const std::string& name, type v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return add##funcname(fd, v); \
    } \
} \
void    PbDynamicMessage::add##funcname(const google::protobuf::FieldDescriptor* field, type v) { \
    m_reflection->Add##pbtype(m_data.get(), field, v); \
}

GEN_GEt_SEt_ADD(Int32Value, Int32, int32_t);
GEN_GEt_SEt_ADD(Uint32Value, UInt32, uint32_t);
GEN_GEt_SEt_ADD(Int64Value, Int64, int64_t);
GEN_GEt_SEt_ADD(Uint64Value, UInt64, uint64_t);
GEN_GEt_SEt_ADD(FloatValue, Float, float);
GEN_GEt_SEt_ADD(DoubleValue, Double, double);
GEN_GEt_SEt_ADD(BoolValue, Bool, bool);
#undef GEN_GEt_SEt_ADD

#define GEN_GEt_SEt_ADD(funcname, pbtype, type) \
type PbDynamicMessage::getRepeated##funcname(const std::string& name, int32_t idx) const { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return getRepeated##funcname(fd, idx); \
    } \
    return 0; \
} \
type PbDynamicMessage::getRepeated##funcname(const google::protobuf::FieldDescriptor* field, int32_t idx) const { \
    return m_reflection->GetRepeated##pbtype(*m_data, field, idx); \
} \
void    PbDynamicMessage::setRepeated##funcname(const std::string& name, int32_t idx, const type& v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return setRepeated##funcname(fd, idx, v); \
    } \
} \
void    PbDynamicMessage::setRepeated##funcname(const google::protobuf::FieldDescriptor* field, int32_t idx,const type& v) { \
    m_reflection->SetRepeated##pbtype(m_data.get(), field, idx, v); \
} \
void    PbDynamicMessage::add##funcname(const std::string& name, const type& v) { \
    auto fd = getFieldByName(name); \
    if(fd) { \
        return add##funcname(fd, v); \
    } \
} \
void    PbDynamicMessage::add##funcname(const google::protobuf::FieldDescriptor* field, const type& v) { \
    m_reflection->Add##pbtype(m_data.get(), field, v); \
}

GEN_GEt_SEt_ADD(StringValue, String, std::string);
#undef GEN_GEt_SEt_ADD



PbDynamicMessage::ptr PbDynamicMessage::mutableMessage(const std::string& name) const {
    auto fd = getFieldByName(name);
    if(fd) {
        return mutableMessage(fd);
    }
    return nullptr;
}

PbDynamicMessage::ptr PbDynamicMessage::mutableMessage(const google::protobuf::FieldDescriptor* field) const {
    auto msg = m_reflection->MutableMessage(m_data.get(), field);
    if(msg) {
        std::shared_ptr<google::protobuf::Message> sm(msg, sylar::nop<google::protobuf::Message>);
        return std::make_shared<PbDynamicMessage>(sm);
    }
    return nullptr;
}

PbDynamicMessage::ptr PbDynamicMessage::mutableRepeatedMessage(const std::string& name, int32_t idx) {
    auto fd = getFieldByName(name);
    if(fd) {
        return mutableRepeatedMessage(fd, idx);
    }
    return nullptr;
}

PbDynamicMessage::ptr PbDynamicMessage::mutableRepeatedMessage(const google::protobuf::FieldDescriptor* field, int32_t idx) {
    auto msg = m_reflection->MutableRepeatedMessage(m_data.get(), field, idx);
    if(msg) {
        std::shared_ptr<google::protobuf::Message> sm(msg, sylar::nop<google::protobuf::Message>);
        return std::make_shared<PbDynamicMessage>(sm);
    }
    return nullptr;
}

PbDynamicMessage::ptr PbDynamicMessage::addMessage(const std::string& name) {
    auto fd = getFieldByName(name);
    if(fd) {
        return addMessage(fd);
    }
    return nullptr;
}

PbDynamicMessage::ptr PbDynamicMessage::addMessage(const google::protobuf::FieldDescriptor* field) {
    auto msg = m_reflection->AddMessage(m_data.get(), field);
    if(msg) {
        std::shared_ptr<google::protobuf::Message> sm(msg, sylar::nop<google::protobuf::Message>);
        return std::make_shared<PbDynamicMessage>(sm);
    }
    return nullptr;
}

int32_t PbDynamicMessage::getDataSize() const {
    return m_reflection->SpaceUsedLong(*m_data);
}

void PbDynamicMessage::addUnknowFieldVarint(int number, uint64_t v) {
    m_reflection->MutableUnknownFields(m_data.get())->AddVarint(number, v);
}

void PbDynamicMessage::addUnknowFieldFixed32(int number, uint32_t v) {
    m_reflection->MutableUnknownFields(m_data.get())->AddFixed32(number, v);
}

void PbDynamicMessage::addUnknowFieldFixed64(int number, uint64_t v) {
    m_reflection->MutableUnknownFields(m_data.get())->AddFixed64(number, v);
}

void PbDynamicMessage::addUnknowFieldFloat(int number, float v) {
    uint32_t m = 0;
    memcpy(&m, &v, sizeof(v));
    addUnknowFieldFixed32(number, m);
}

void PbDynamicMessage::addUnknowFieldDouble(int number, double v) {
    uint64_t m = 0;
    memcpy(&m, &v, sizeof(v));
    addUnknowFieldFixed64(number, m);
}

void PbDynamicMessage::addUnknowFieldLengthDelimited(int number, const std::string& v) {
    m_reflection->MutableUnknownFields(m_data.get())->AddLengthDelimited(number, v);
}

void PbDynamicMessage::addUnknowFieldString(int number, const std::string& v) {
    m_reflection->MutableUnknownFields(m_data.get())->AddLengthDelimited(number, v);
}

void PbDynamicMessage::addUnknowFieldMessage(int number, const google::protobuf::Message& v) {
    std::string tmp;
    v.SerializeToString(&tmp);
    addUnknowFieldLengthDelimited(number, tmp);
}

void PbDynamicMessage::delUnknowField(int number) {
    m_reflection->MutableUnknownFields(m_data.get())->DeleteByNumber(number);
}

void PbDynamicMessageProto::setName(const std::string& v) {
    m_data.set_name(v);
}

const std::string& PbDynamicMessageProto::getName() const {
    return m_data.name();
}

void PbDynamicMessageProto::addField(int32_t number, const std::string& name, int type,  int label) {
    auto f = m_data.add_field();
    f->set_name(name);
    f->set_number(number);
    f->set_type((google::protobuf::FieldDescriptorProto::Type)type);
    f->set_label((google::protobuf::FieldDescriptorProto::Label)label);
}

#define ADD_FIELD(funcname, type) \
void PbDynamicMessageProto::add##funcname(int32_t number, const std::string& name, int label) { \
    addField(number, name, type, label); \
}
ADD_FIELD(DoubleField,      TYPE_DOUBLE);  
ADD_FIELD(FloatField,       TYPE_FLOAT);
ADD_FIELD(Int64Field,       TYPE_INT64);
ADD_FIELD(Uint64Field,      TYPE_UINT64);
ADD_FIELD(Int32Field,       TYPE_INT32);
ADD_FIELD(Fixed64Field,     TYPE_FIXED64);
ADD_FIELD(Fixed32Field,     TYPE_FIXED32);
ADD_FIELD(BoolField,        TYPE_BOOL);
ADD_FIELD(StringField,      TYPE_STRING);
ADD_FIELD(GroupField,       TYPE_GROUP);
ADD_FIELD(BytesField,       TYPE_BYTES);
ADD_FIELD(Uint32Field,      TYPE_UINT32);
ADD_FIELD(EnumField,        TYPE_ENUM);
ADD_FIELD(Sfixed32Field,    TYPE_SFIXED32);
ADD_FIELD(Sfixed64Field,    TYPE_SFIXED64);
ADD_FIELD(Sint32Field,      TYPE_SINT32);
ADD_FIELD(Sint64Field,      TYPE_SINT64);
#undef ADD_FIELD


void PbDynamicMessageProto::addMessageField(int32_t number, const std::string& name, const std::string& message_name, int label) {
    auto f = m_data.add_field();
    f->set_name(name);
    f->set_number(number);
    f->set_type(google::protobuf::FieldDescriptorProto::TYPE_MESSAGE);
    f->set_label((google::protobuf::FieldDescriptorProto::Label)label);
    f->set_type_name(message_name);
}

PbDynamicMessageFactory::PbDynamicMessageFactory() {
    m_sourceTree = std::make_shared<google::protobuf::compiler::DiskSourceTree>();
    m_database = std::make_shared<google::protobuf::compiler::SourceTreeDescriptorDatabase>(m_sourceTree.get());
    m_pool = std::make_shared<google::protobuf::DescriptorPool>(m_database.get());
    m_factory = std::make_shared<google::protobuf::DynamicMessageFactory>(m_pool.get());
}

static const std::string EMPTY_MESSAGE_NAME = "EmptyMessage";

std::shared_ptr<google::protobuf::Message> PbDynamicMessageFactory::createEmptyMessage() {
    return createMessageByName(EMPTY_MESSAGE_NAME);
}

PbDynamicMessage::ptr PbDynamicMessageFactory::createDynamicEmptyMessage() {
    return createDynamicMessageByName(EMPTY_MESSAGE_NAME);
}

PbDynamicMessageFactory::ptr PbDynamicMessageFactory::CreateWithUnderlay(PbDynamicMessageFactory::ptr factory) {
    auto rt = std::make_shared<PbDynamicMessageFactory>();
    if(!factory) {
        factory = std::make_shared<PbDynamicMessageFactory>();
    }
    rt->m_sourceTree = factory->m_sourceTree;
    rt->m_pool = std::make_shared<google::protobuf::DescriptorPool>(factory->m_pool.get());
    rt->m_factory = std::make_shared<google::protobuf::DynamicMessageFactory>(rt->m_pool.get());
    rt->m_underlay = factory;

    auto proto = std::make_shared<PbDynamicMessageProto>();
    proto->setName(EMPTY_MESSAGE_NAME);
    rt->addPbDynamicProto(proto);
    return rt;
}

void PbDynamicMessageFactory::addProtoPathMap(const std::string& virtual_path, const std::string& disk_path) {
    if(m_addPath.count(std::make_pair(virtual_path, disk_path)) == 0) {
        m_sourceTree->MapPath(virtual_path, disk_path);
    }
}

PbDynamicMessageFactory::~PbDynamicMessageFactory() {
    SYLAR_LOG_INFO(g_logger) << "PbDynamicMessageFactory::~PbDynamicMessageFactory";
}

void PbDynamicMessageFactory::addProtoPathMap(const std::vector<std::pair<std::string, std::string> >& paths) {
    for(auto& i : paths) {
        addProtoPathMap(i.first, i.second);
    }
}

const google::protobuf::FileDescriptor* PbDynamicMessageFactory::findFileByName(const std::string& path) {
    return m_pool->FindFileByName(path);
}

const google::protobuf::Descriptor* PbDynamicMessageFactory::findMessageTypeByName(const std::string& name) {
    return m_pool->FindMessageTypeByName(name);
}

template<class T, class D>
struct DeleteWithData {
    DeleteWithData(D d)
        :data(d) {
    }
    void operator()(T* ptr) {
        delete ptr;
    }

    D data;
};

std::shared_ptr<google::protobuf::Message> PbDynamicMessageFactory::createMessageByName(const std::string& name) {
    auto type = findMessageTypeByName(name);
    if(!type) {
        return nullptr;
    }
    SYLAR_LOG_INFO(g_logger) << "type.name=" << type->name() << " - " << type->full_name();
    return std::shared_ptr<google::protobuf::Message>(m_factory->GetPrototype(type)->New(),
            DeleteWithData<google::protobuf::Message, ptr>(shared_from_this()));
}

PbDynamicMessage::ptr PbDynamicMessageFactory::createDynamicMessageByName(const std::string& name) {
    auto msg = createMessageByName(name);
    if(msg) {
        return std::make_shared<PbDynamicMessage>(msg);
    }
    return nullptr;
}

std::vector<const google::protobuf::FileDescriptor*> PbDynamicMessageFactory::loadDir(const std::string& dir) {
    addProtoPathMap("", dir);
    std::vector<std::string> files;
    std::vector<const google::protobuf::FileDescriptor*> rt;
    sylar::FSUtil::ListAllFile(files, dir, ".proto");
    for(auto& i : files) {
        auto name = i.substr(dir.size() + 1);
        auto v = findFileByName(name);
        if(v) {
            rt.push_back(v);
        } else {
            SYLAR_LOG_WARN(g_logger) << "findFileByName " << name << " fail";
        }
    }
    return rt;
}

const google::protobuf::FileDescriptor* PbDynamicMessageFactory::addPbDynamicProto(PbDynamicMessageProto::ptr proto, const std::string& filename, const std::vector<std::string>& imports) {
    google::protobuf::FileDescriptorProto file;

    file.set_syntax("proto3");
    auto f = file.add_message_type();
    *f = proto->getData();

    for(auto& i : imports) {
        file.add_dependency(i);
    }

    std::string full_name;
    if(!filename.empty()) {
        full_name = filename;
    } else {
        full_name = proto->getPackage().empty() ? proto->getName() : proto->getPackage() + "." + proto->getName() + ".proto";
    }
    file.set_name(full_name);
    return m_pool->BuildFile(file);
}

PbDynamicMessageFactoryManager::PbDynamicMessageFactoryManager() {
    m_default = std::make_shared<PbDynamicMessageFactory>();
    m_default->m_pool.reset((google::protobuf::DescriptorPool*)google::protobuf::DescriptorPool::generated_pool()
                            ,sylar::nop<google::protobuf::DescriptorPool>);
    m_default->m_factory = std::make_shared<google::protobuf::DynamicMessageFactory>
                           (m_default->m_pool.get());
}

PbDynamicMessageFactory::ptr PbDynamicMessageFactoryManager::getDefault() const {
    return m_default;
}

PbDynamicMessageFactory::ptr PbDynamicMessageFactoryManager::get(const std::string& name) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(name);
    return it == m_datas.end() ? nullptr : it->second;
}

void PbDynamicMessageFactoryManager::add(const std::string& name, PbDynamicMessageFactory::ptr v) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_datas[name] = v;
}

}
