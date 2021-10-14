#ifndef __SYLAR_UTIL_PB_DYNAMIC_MESSAGE_H__
#define __SYLAR_UTIL_PB_DYNAMIC_MESSAGE_H__

#include <google/protobuf/message.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/compiler/importer.h>

#include "sylar/mutex.h"
#include "sylar/singleton.h"
#include <set>

namespace sylar {

std::string ProtoToJson(const google::protobuf::Message& m);
bool JsonToProto(const std::string& json, google::protobuf::Message& m);

class PbDynamicMessage {
public:
    typedef std::shared_ptr<PbDynamicMessage> ptr;
    PbDynamicMessage(std::shared_ptr<google::protobuf::Message> data);

    std::shared_ptr<google::protobuf::Message> getData() const { return m_data;}

    const google::protobuf::FieldDescriptor* getFieldByName(const std::string& name) const;

    // non repeated field is set
    bool hasFieldValue(const std::string& name) const;
    bool hasFieldValue(const google::protobuf::FieldDescriptor* field) const;

    // repeated field size
    int32_t getFieldValueSize(const std::string& name) const;
    int32_t getFieldValueSize(const google::protobuf::FieldDescriptor* field) const;

    //after hasFieldValue return false or getFieldValueSzie return 0
    void clearFieldValue(const std::string& name);
    void clearFieldValue(const google::protobuf::FieldDescriptor* field);

    //hasFieldValue return true or getFieldValueSize > 0
    std::vector<const google::protobuf::FieldDescriptor*> listFields();

    //non-repeated int32 get/set
    int32_t getInt32Value(const std::string& name) const;
    int32_t getInt32Value(const google::protobuf::FieldDescriptor* field) const;
    void setInt32Value(const std::string& name, int32_t v);
    void setInt32Value(const google::protobuf::FieldDescriptor* field, int32_t v);

    //non-repeated int64 get/set
    int64_t getInt64Value(const std::string& name) const;
    int64_t getInt64Value(const google::protobuf::FieldDescriptor* field) const;
    void setInt64Value(const std::string& name, int64_t v);
    void setInt64Value(const google::protobuf::FieldDescriptor* field, int64_t v);

    //non-repeated uint32 get/set
    uint32_t getUint32Value(const std::string& name) const;
    uint32_t getUint32Value(const google::protobuf::FieldDescriptor* field) const;
    void setUint32Value(const std::string& name, uint32_t v);
    void setUint32Value(const google::protobuf::FieldDescriptor* field, uint32_t v);

    //non-repeated uint64 get/set
    uint64_t getUint64Value(const std::string& name) const;
    uint64_t getUint64Value(const google::protobuf::FieldDescriptor* field) const;
    void setUint64Value(const std::string& name, uint64_t v);
    void setUint64Value(const google::protobuf::FieldDescriptor* field, uint64_t v);

    //non-repeated float get/set
    float getFloatValue(const std::string& name) const;
    float getFloatValue(const google::protobuf::FieldDescriptor* field) const;
    void setFloatValue(const std::string& name, float v);
    void setFloatValue(const google::protobuf::FieldDescriptor* field, float v);

    //non-repeated double get/set
    double getDoubleValue(const std::string& name) const;
    double getDoubleValue(const google::protobuf::FieldDescriptor* field) const;
    void setDoubleValue(const std::string& name, double v);
    void setDoubleValue(const google::protobuf::FieldDescriptor* field, double v);

    //non-repeated bool get/set
    bool getBoolValue(const std::string& name) const;
    bool getBoolValue(const google::protobuf::FieldDescriptor* field) const;
    void setBoolValue(const std::string& name, bool v);
    void setBoolValue(const google::protobuf::FieldDescriptor* field, bool v);

    //non-repeated string get/set
    std::string getStringValue(const std::string& name) const;
    std::string getStringValue(const google::protobuf::FieldDescriptor* field) const;
    void setStringValue(const std::string& name, const std::string& v);
    void setStringValue(const google::protobuf::FieldDescriptor* field, const std::string& v);

    //non-repeated message
    PbDynamicMessage::ptr mutableMessage(const std::string& name) const;
    PbDynamicMessage::ptr mutableMessage(const google::protobuf::FieldDescriptor* field) const;

    //repeated int32 get/set/add
    int32_t getRepeatedInt32Value(const std::string& name, int32_t idx) const;
    int32_t getRepeatedInt32Value(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void    setRepeatedInt32Value(const std::string& name, int32_t idx, int32_t v);
    void    setRepeatedInt32Value(const google::protobuf::FieldDescriptor* field, int32_t idx, int32_t v);
    void    addInt32Value(const std::string& name, int32_t v);
    void    addInt32Value(const google::protobuf::FieldDescriptor* field, int32_t v);

    //repeated uint32 get/set/add
    uint32_t getRepeatedUint32Value(const std::string& name, int32_t idx) const;
    uint32_t getRepeatedUint32Value(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void     setRepeatedUint32Value(const std::string& name, int32_t idx, uint32_t v);
    void     setRepeatedUint32Value(const google::protobuf::FieldDescriptor* field, int32_t idx, uint32_t v);
    void     addUint32Value(const std::string& name, uint32_t v);
    void     addUint32Value(const google::protobuf::FieldDescriptor* field, uint32_t v);

    //repeated int64 get/set/add
    int64_t getRepeatedInt64Value(const std::string& name, int32_t idx) const;
    int64_t getRepeatedInt64Value(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void    setRepeatedInt64Value(const std::string& name, int32_t idx, int64_t v);
    void    setRepeatedInt64Value(const google::protobuf::FieldDescriptor* field, int32_t idx, int64_t v);
    void    addInt64Value(const std::string& name, int64_t v);
    void    addInt64Value(const google::protobuf::FieldDescriptor* field, int64_t v);

    //repeated uint64 get/set/add
    uint64_t getRepeatedUint64Value(const std::string& name, int32_t idx) const;
    uint64_t getRepeatedUint64Value(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void     setRepeatedUint64Value(const std::string& name, int32_t idx, uint64_t v);
    void     setRepeatedUint64Value(const google::protobuf::FieldDescriptor* field, int32_t idx, uint64_t v);
    void     addUint64Value(const std::string& name, uint64_t v);
    void     addUint64Value(const google::protobuf::FieldDescriptor* field, uint64_t v);

    //repeated float get/set/add
    float getRepeatedFloatValue(const std::string& name, int32_t idx) const;
    float getRepeatedFloatValue(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void  setRepeatedFloatValue(const std::string& name, int32_t idx, float v);
    void  setRepeatedFloatValue(const google::protobuf::FieldDescriptor* field, int32_t idx, float v);
    void  addFloatValue(const std::string& name, float v);
    void  addFloatValue(const google::protobuf::FieldDescriptor* field, float v);

    //repeated double get/set/add
    double getRepeatedDoubleValue(const std::string& name, int32_t idx) const;
    double getRepeatedDoubleValue(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void   setRepeatedDoubleValue(const std::string& name, int32_t idx, double v);
    void   setRepeatedDoubleValue(const google::protobuf::FieldDescriptor* field, int32_t idx, double v);
    void   addDoubleValue(const std::string& name, double v);
    void   addDoubleValue(const google::protobuf::FieldDescriptor* field, double v);

    //repeated bool get/set/add
    bool getRepeatedBoolValue(const std::string& name, int32_t idx) const;
    bool getRepeatedBoolValue(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void setRepeatedBoolValue(const std::string& name, int32_t idx, bool v);
    void setRepeatedBoolValue(const google::protobuf::FieldDescriptor* field, int32_t idx, bool v);
    void addBoolValue(const std::string& name, bool v);
    void addBoolValue(const google::protobuf::FieldDescriptor* field, bool v);

    //repeated string get/set/add
    std::string getRepeatedStringValue(const std::string& name, int32_t idx) const;
    std::string getRepeatedStringValue(const google::protobuf::FieldDescriptor* field, int32_t idx) const;
    void        setRepeatedStringValue(const std::string& name, int32_t idx, const std::string& v);
    void        setRepeatedStringValue(const google::protobuf::FieldDescriptor* field, int32_t idx, const std::string& v);
    void        addStringValue(const std::string& name, const std::string& v);
    void        addStringValue(const google::protobuf::FieldDescriptor* field, const std::string& v);

    //repeated message
    PbDynamicMessage::ptr mutableRepeatedMessage(const std::string& name, int32_t idx);
    PbDynamicMessage::ptr mutableRepeatedMessage(const google::protobuf::FieldDescriptor* field, int32_t idx);
    PbDynamicMessage::ptr addMessage(const std::string& name);
    PbDynamicMessage::ptr addMessage(const google::protobuf::FieldDescriptor* field);

    int32_t getDataSize() const;

    //unknowfields 
    void addUnknowFieldVarint(int number, uint64_t v);
    void addUnknowFieldFixed32(int number, uint32_t v);
    void addUnknowFieldFixed64(int number, uint64_t v);
    void addUnknowFieldFloat(int number, float v);
    void addUnknowFieldDouble(int number, double v);
    void addUnknowFieldLengthDelimited(int number, const std::string& v);
    void addUnknowFieldString(int number, const std::string& v);
    void addUnknowFieldMessage(int number, const google::protobuf::Message& v);
    void delUnknowField(int number);
private:
    std::shared_ptr<google::protobuf::Message> m_data;
    const google::protobuf::Reflection* m_reflection;
    const google::protobuf::Descriptor* m_descriptor;
};

class PbDynamicMessageProto {
public:
    typedef std::shared_ptr<PbDynamicMessageProto> ptr;
    enum Label {
        OPTIONAL = 1,
        REQUIRED = 2,
        REPEATED = 3
    };

    enum Type {
        // 0 is reserved for errors.
        // Order is weird for historical reasons.
        TYPE_DOUBLE = 1,
        TYPE_FLOAT = 2,
        // Not ZigZag encoded.  Negative numbers take 10 bytes.  Use TYPE_SINT64 if
        // negative values are likely.
        TYPE_INT64 = 3,
        TYPE_UINT64 = 4,
        // Not ZigZag encoded.  Negative numbers take 10 bytes.  Use TYPE_SINT32 if
        // negative values are likely.
        TYPE_INT32 = 5,
        TYPE_FIXED64 = 6,
        TYPE_FIXED32 = 7,
        TYPE_BOOL = 8,
        TYPE_STRING = 9,
        // Tag-delimited aggregate.
        // Group type is deprecated and not supported in proto3. However, Proto3
        // implementations should still be able to parse the group wire format and
        // treat group fields as unknown fields.
        TYPE_GROUP = 10,
        TYPE_MESSAGE = 11, // Length-delimited aggregate.
        
        // New in version 2.
        TYPE_BYTES = 12,
        TYPE_UINT32 = 13,
        TYPE_ENUM = 14,
        TYPE_SFIXED32 = 15,
        TYPE_SFIXED64 = 16,
        TYPE_SINT32 = 17,  // Uses ZigZag encoding.
        TYPE_SINT64 = 18,  // Uses ZigZag encoding.
    };

    void setName(const std::string& v);
    const std::string& getName() const;
    void setPackage(const std::string& v) { m_package = v;}
    std::string getPackage() const { return m_package;}
    const google::protobuf::DescriptorProto& getData() const { return m_data;}

    void addField(int32_t number, const std::string& name, int type,  int label = OPTIONAL);

    void addDoubleField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addFloatField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addInt64Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addUint64Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addInt32Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addFixed64Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addFixed32Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addBoolField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addStringField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addGroupField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addMessageField(int32_t number, const std::string& name, const std::string& message_name, int label = OPTIONAL);
    void addBytesField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addUint32Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addEnumField(int32_t number, const std::string& name, int label = OPTIONAL);
    void addSfixed32Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addSfixed64Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addSint32Field(int32_t number, const std::string& name, int label = OPTIONAL);
    void addSint64Field(int32_t number, const std::string& name, int label = OPTIONAL);
private:
    std::string m_package;
    google::protobuf::DescriptorProto m_data;
};

class PbDynamicMessageFactoryManager;
class PbDynamicMessageFactory : public std::enable_shared_from_this<PbDynamicMessageFactory> {
friend class PbDynamicMessageFactoryManager;
public:
    typedef std::shared_ptr<PbDynamicMessageFactory> ptr;
    static PbDynamicMessageFactory::ptr CreateWithUnderlay(PbDynamicMessageFactory::ptr factory = nullptr);

    PbDynamicMessageFactory();
    ~PbDynamicMessageFactory();

    void addProtoPathMap(const std::string& virtual_path, const std::string& disk_path);
    void addProtoPathMap(const std::vector<std::pair<std::string, std::string> >& paths);

    const google::protobuf::FileDescriptor* findFileByName(const std::string& path);
    std::vector<const google::protobuf::FileDescriptor*> loadDir(const std::string& dir);

    const google::protobuf::Descriptor* findMessageTypeByName(const std::string& name);
    std::shared_ptr<google::protobuf::Message> createMessageByName(const std::string& name);
    PbDynamicMessage::ptr createDynamicMessageByName(const std::string& name);

    std::shared_ptr<google::protobuf::Message> createEmptyMessage();
    PbDynamicMessage::ptr createDynamicEmptyMessage();

    const google::protobuf::FileDescriptor* addPbDynamicProto(PbDynamicMessageProto::ptr proto, const std::string& filename = "", const std::vector<std::string>& imports = {});
private:
    std::shared_ptr<google::protobuf::compiler::DiskSourceTree> m_sourceTree;
    std::shared_ptr<google::protobuf::compiler::SourceTreeDescriptorDatabase> m_database;
    std::shared_ptr<google::protobuf::DescriptorPool> m_pool;
    std::shared_ptr<google::protobuf::DynamicMessageFactory> m_factory;
    std::set<std::pair<std::string, std::string> > m_addPath;
    PbDynamicMessageFactory::ptr m_underlay;
};

class PbDynamicMessageFactoryManager {
public:
    PbDynamicMessageFactoryManager();

    PbDynamicMessageFactory::ptr getDefault() const;
    PbDynamicMessageFactory::ptr get(const std::string& name);
    void add(const std::string& name, PbDynamicMessageFactory::ptr v);
private:
    sylar::RWMutex m_mutex;
    PbDynamicMessageFactory::ptr m_default;
    std::unordered_map<std::string, PbDynamicMessageFactory::ptr> m_datas;
};

typedef sylar::Singleton<PbDynamicMessageFactoryManager> PbDynamicMessageFactoryMgr;

}

#endif
