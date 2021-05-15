#ifndef __SYLAR_HTTP2_HPACK_H__
#define __SYLAR_HTTP2_HPACK_H__

#include "sylar/bytearray.h"
#include "dynamic_table.h"

namespace sylar {
namespace http2 {

enum class IndexType {
    INDEXED                         = 0,
    WITH_INDEXING_INDEXED_NAME      = 1,
    WITH_INDEXING_NEW_NAME          = 2,
    WITHOUT_INDEXING_INDEXED_NAME   = 3,
    WITHOUT_INDEXING_NEW_NAME       = 4,
    NERVER_INDEXED_INDEXED_NAME      = 5,
    NERVER_INDEXED_NEW_NAME         = 6,
    ERROR                           = 7
};

std::string IndexTypeToString(IndexType type);

struct StringHeader {
    union {
        struct {
            uint8_t len : 7;
            uint8_t h : 1;
        };
        uint8_t h_len;
    };
};

struct FieldHeader {
    union {
        struct {
            uint8_t index : 7;
            uint8_t code : 1;
        } indexed;
        struct {
            uint8_t index : 6;
            uint8_t code : 2;
        } with_indexing;
        struct {
            uint8_t index : 4;
            uint8_t code : 4;
        } other;
        uint8_t type = 0;
    };
};

struct HeaderField {
    IndexType type = IndexType::ERROR;
    bool h_name = 0;
    bool h_value = 0;
    uint32_t index = 0;
    std::string name;
    std::string value;

    std::string toString() const;
};

class HPack {
public:
    HPack(DynamicTable& table);

    int parse(ByteArray::ptr ba, int length);
    int parse(std::string& data);
    int pack(HeaderField* header, ByteArray::ptr ba);
    int pack(const std::vector<std::pair<std::string, std::string> >& headers, ByteArray::ptr ba);
    int pack(const std::vector<std::pair<std::string, std::string> >& headers, std::string& out);

    std::vector<HeaderField>& getHeaders() { return m_headers;}
    static int Pack(HeaderField* header, ByteArray::ptr ba);

    std::string toString() const;
public:
    static int WriteVarInt(ByteArray::ptr ba, int32_t prefix, uint64_t value, uint8_t flags);
    static uint64_t ReadVarInt(ByteArray::ptr ba, int32_t prefix);
    static uint64_t ReadVarInt(ByteArray::ptr ba, uint8_t b, int32_t prefix);
    static std::string ReadString(ByteArray::ptr ba);
    static int WriteString(ByteArray::ptr ba, const std::string& str, bool h);

private:
    std::vector<HeaderField> m_headers;
    DynamicTable& m_table;
};


}
}

#endif
