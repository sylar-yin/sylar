#include "parquet.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


ArrowListArray::ArrowListArray(std::shared_ptr<arrow::ListArray> data) 
    :m_data(data) {
}

int32_t ArrowListArray::getLength() const {
    return m_data->length();
}
std::shared_ptr<arrow::Array> ArrowListArray::getValue(int32_t idx) const {
    return m_data->value_slice(idx);
}
bool ArrowListArray::isNull(int32_t idx) const {
    return m_data->IsNull(idx);
}

ArrowChunkedArray::ArrowChunkedArray(std::shared_ptr<arrow::ChunkedArray> data)
    :m_data(data) {
}

std::string ArrowChunkedArray::toString() const {
    return m_data->ToString();
}

int64_t ArrowChunkedArray::getLength() const {
    return m_data->length();
}

int32_t ArrowChunkedArray::getNullCount() const {
    return m_data->null_count();
}

int32_t ArrowChunkedArray::getNumChunks() const {
    return m_data->num_chunks();
}

ArrowChunkedArray::ptr ArrowChunkedArray::slice(int64_t offset) const {
    return std::make_shared<ArrowChunkedArray>(m_data->Slice(offset));
}

ArrowChunkedArray::ptr ArrowChunkedArray::slice(int64_t offset, int64_t length) const {
    return std::make_shared<ArrowChunkedArray>(m_data->Slice(offset, length));
}

std::shared_ptr<arrow::Array> ArrowChunkedArray::chunk(int32_t idx) const {
    return m_data->chunk(idx);
}

ArrowListArray::ptr ArrowChunkedArray::chunkListType(int32_t idx) const {
    return std::make_shared<ArrowListArray>(chunkAs<arrow::ListArray>(idx));
}

ArrowSchema::ArrowSchema(std::shared_ptr<arrow::Schema> schema)
    :m_data(schema) {
}

std::string ArrowSchema::toString() const {
    return m_data->ToString();
}

std::shared_ptr<arrow::Field> ArrowSchema::getField(int32_t idx) const {
    return m_data->field(idx);
}

std::shared_ptr<arrow::Field> ArrowSchema::getFieldByName(const std::string& name) const {
    return m_data->GetFieldByName(name);
}

int32_t ArrowSchema::getFieldIndex(const std::string& name) const {
    return m_data->GetFieldIndex(name);
}

ArrowTable::ArrowTable(std::shared_ptr<arrow::Table> tab)
    :m_data(tab) {
}

std::string ArrowTable::toString() const {
    return m_data->ToString();
}

ArrowSchema::ptr ArrowTable::getSchema() const {
    return std::make_shared<ArrowSchema>(m_data->schema());
}

int32_t ArrowTable::getRows() const {
    return m_data->num_rows();
}

int32_t ArrowTable::getCols() const {
    return m_data->num_columns();
}

ArrowTable::ptr ArrowTable::slice(int64_t offset) const {
    return std::make_shared<ArrowTable>(m_data->Slice(offset));
}

ArrowTable::ptr ArrowTable::slice(int64_t offset, int64_t length) const {
    return std::make_shared<ArrowTable>(m_data->Slice(offset, length));
}

ArrowTable::ptr ArrowTable::selectColumns(const std::vector<int32_t>& indices) const {
    auto rt = m_data->SelectColumns(indices);
    if(rt.ok()) {
        return std::make_shared<ArrowTable>(rt.ValueOrDie());
    }
    SYLAR_LOG_ERROR(g_logger) << "selectColumns fail, " << rt.status().message();
    return nullptr;
}

ArrowTable::ptr ArrowTable::selectColumns(const std::vector<std::string>& cols) const {
    std::vector<int32_t> indices;
    auto schema = getSchema();
    for(auto& i : cols) {
        indices.push_back(schema->getFieldIndex(i));
    }
    return selectColumns(indices);
}

ArrowChunkedArray::ptr ArrowTable::getColumnData(int32_t index) const {
    return std::make_shared<ArrowChunkedArray>(m_data->column(index));
}

ArrowChunkedArray::ptr ArrowTable::getColumnData(const std::string& name) const {
    return std::make_shared<ArrowChunkedArray>(m_data->GetColumnByName(name));
}

ParquetFileReader::ptr ParquetFileReader::Open(const std::string& filename) {
    try {
        auto rt = std::make_shared<ParquetFileReader>();
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(
            infile,
            arrow::io::ReadableFile::Open(filename,
                                        arrow::default_memory_pool()));
        PARQUET_THROW_NOT_OK(
            parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &rt->m_data));
        return rt;
    } catch (std::exception& e) {
        SYLAR_LOG_ERROR(g_logger) << "ParquetFileReader Open " << filename << " fail, "
            << e.what();
    }
    return nullptr;
}

ArrowSchema::ptr ParquetFileReader::getSchema() const {
    std::shared_ptr<arrow::Schema> schema;
    m_data->GetSchema(&schema);
    return std::make_shared<ArrowSchema>(schema);
}

ArrowTable::ptr ParquetFileReader::readTable() const {
    std::shared_ptr<::arrow::Table> table;
    m_data->ReadTable(&table);
    return std::make_shared<ArrowTable>(table);
}

ArrowTable::ptr ParquetFileReader::readTable(const std::vector<int32_t>& indices) const {
    std::shared_ptr<::arrow::Table> table;
    m_data->ReadTable(indices, &table);
    return std::make_shared<ArrowTable>(table);
}

ArrowTable::ptr ParquetFileReader::readTable(const std::vector<std::string>& colnames) const {
    std::vector<int32_t> indices;
    for(auto& i : colnames) {
        indices.push_back(getSchema()->getFieldIndex(i));
    }
    return readTable(indices);
}

std::shared_ptr<parquet::FileMetaData> ParquetFileReader::getMetadata() const {
    return m_data->parquet_reader()->metadata();
}

std::string ParquetFileReader::toString() const {
    auto meta = getMetadata();
    std::stringstream ss;
    ss << "num_cols=" << meta->num_columns()
       << " num_schema_elements=" << meta->num_schema_elements()
       << " num_rows=" << meta->num_rows()
       << " num_row_groups=" << meta->num_row_groups()
       << " version=" << meta->version()
       << " created_by=" << meta->created_by()
       //<< " writer_version=" << meta->writer_version()
       << std::endl;
    auto schema = meta->schema();
    ss << schema->ToString() << std::endl;
    return ss.str();
}

ArrowTableBuilder::ArrowTableBuilder() {
}

int32_t ArrowTableBuilder::getColumnIndex(const std::string& name) const {
    auto it = m_name2indexs.find(name);
    return it == m_name2indexs.end() ? -1 : it->second;
}

std::shared_ptr<arrow::ArrayBuilder> ArrowTableBuilder::getColumn(int32_t idx) const {
    return m_builders[idx];
}

std::shared_ptr<arrow::ArrayBuilder> ArrowTableBuilder::getColumn(const std::string& name) const {
    auto idx = getColumnIndex(name);
    if(idx == -1) {
        return nullptr;
    }
    return getColumn(idx);
}

int32_t ArrowTableBuilder::addColumn(const std::string& name, std::shared_ptr<arrow::DataType> type, bool nullable) {
    auto idx = getColumnIndex(name);
    if(idx != -1) {
        return -1;
    }

    m_name2indexs[name] = m_fields.size();
    m_fields.push_back(arrow::field(name, type, nullable));
    switch(type->id()) {
#define XX(m, b) \
        case arrow::Type::m: \
            m_builders.push_back(std::make_shared<arrow::b##Builder>(type, arrow::default_memory_pool())); \
            break;
    XX(NA,      Null);
    XX(BOOL,    Boolean);
    XX(UINT8,   UInt8);
    XX(INT8,    Int8);
    XX(UINT16,  UInt16);
    XX(INT16,   Int16);
    XX(UINT32,  UInt32);
    XX(INT32,   Int32);
    XX(UINT64,  UInt64);
    XX(INT64,   Int64);
    XX(HALF_FLOAT,HalfFloat);
    XX(FLOAT,   Float);
    XX(DOUBLE,  Double);
    XX(STRING,  String);
    XX(BINARY,  Binary);
    XX(FIXED_SIZE_BINARY,   FixedSizeBinary);
    XX(DATE32,  Date32);
    XX(DATE64,  Date64);
    XX(TIMESTAMP,   Timestamp);
    XX(TIME32,  Time32);
    XX(TIME64,  Time64);
    //XX(INTERVAL_MONTHS, IntervalMonths);
    //XX(INTERVAL_DAY_TIME,IntervalDayTime);
    XX(DECIMAL128,  Decimal128);
    XX(DECIMAL256,  Decimal256);
    //XX(LIST,    List);
    //XX(STRUCT,  Struct);
    //XX(SPARSE_UNION,    SparseUnion);
    //XX(DENSE_UNION, DenseUnion);
    //XX(DICTIONARY,  Dictionary);
    //XX(MAP, Map);
    //XX(EXTENSION,   Extension);
    //XX(FIXED_SIZE_LIST, FixedSizeList);
    XX(DURATION,    Duration);
    XX(LARGE_STRING,    LargeString);
    XX(LARGE_BINARY,    LargeBinary);
    //XX(LARGE_LIST,      LargeList);
    //XX(INTERVAL_MONTH_DAY_NANO, IntervalMonthDayNano);
#undef XX
        default:
            m_builders.push_back(nullptr);
            break;
    }
    return m_fields.size() - 1;
}

int32_t ArrowTableBuilder::addListColumn(const std::string& name, std::shared_ptr<arrow::DataType> type, bool list_nullable, bool value_nullable) {
    auto idx = getColumnIndex(name);
    if(idx != -1) {
        return -1;
    }

    m_name2indexs[name] = m_fields.size();
    auto field = arrow::field("item", type, value_nullable);
    m_fields.push_back(arrow::field(name, arrow::list(field), list_nullable));
    switch(type->id()) {
#define XX(m, b) \
        case arrow::Type::m: \
            m_builders.push_back(std::make_shared<arrow::ListBuilder>(arrow::default_memory_pool(), \
                    std::make_shared<arrow::b##Builder>(type, arrow::default_memory_pool()), arrow::list(field))); \
            break;
    XX(NA,      Null);
    XX(BOOL,    Boolean);
    XX(UINT8,   UInt8);
    XX(INT8,    Int8);
    XX(UINT16,  UInt16);
    XX(INT16,   Int16);
    XX(UINT32,  UInt32);
    XX(INT32,   Int32);
    XX(UINT64,  UInt64);
    XX(INT64,   Int64);
    XX(HALF_FLOAT,HalfFloat);
    XX(FLOAT,   Float);
    XX(DOUBLE,  Double);
    XX(STRING,  String);
    XX(BINARY,  Binary);
    XX(FIXED_SIZE_BINARY,   FixedSizeBinary);
    XX(DATE32,  Date32);
    XX(DATE64,  Date64);
    XX(TIMESTAMP,   Timestamp);
    XX(TIME32,  Time32);
    XX(TIME64,  Time64);
    //XX(INTERVAL_MONTHS, IntervalMonths);
    //XX(INTERVAL_DAY_TIME,IntervalDayTime);
    XX(DECIMAL128,  Decimal128);
    XX(DECIMAL256,  Decimal256);
    //XX(LIST,    List);
    //XX(STRUCT,  Struct);
    //XX(SPARSE_UNION,    SparseUnion);
    //XX(DENSE_UNION, DenseUnion);
    //XX(DICTIONARY,  Dictionary);
    //XX(MAP, Map);
    //XX(EXTENSION,   Extension);
    //XX(FIXED_SIZE_LIST, FixedSizeList);
    XX(DURATION,    Duration);
    XX(LARGE_STRING,    LargeString);
    XX(LARGE_BINARY,    LargeBinary);
    //XX(LARGE_LIST,      LargeList);
    //XX(INTERVAL_MONTH_DAY_NANO, IntervalMonthDayNano);
#undef XX
        default:
            m_builders.push_back(nullptr);
            break;
    }
    return m_fields.size() - 1;
}


int32_t ArrowTableBuilder::addMapColumn(const std::string& name, std::shared_ptr<arrow::DataType> key_type,
                         std::shared_ptr<arrow::DataType> value_type, bool nullable, bool key_sorted) {
    auto idx = getColumnIndex(name);
    if(idx != -1) {
        return -1;
    }

    m_name2indexs[name] = m_fields.size();
    //auto field = arrow::field("item", type, value_nullable);
    //m_fields.push_back(arrow::field(name, arrow::list(field), list_nullable));
    m_fields.push_back(arrow::field(name, arrow::map(key_type, value_type, key_sorted), nullable));
    std::shared_ptr<arrow::ArrayBuilder> key_builder;
    std::shared_ptr<arrow::ArrayBuilder> val_builder;

#define TYPE_LIST(XX) \
    XX(NA,      Null); \
    XX(BOOL,    Boolean); \
    XX(UINT8,   UInt8); \
    XX(INT8,    Int8); \
    XX(UINT16,  UInt16); \
    XX(INT16,   Int16); \
    XX(UINT32,  UInt32); \
    XX(INT32,   Int32); \
    XX(UINT64,  UInt64); \
    XX(INT64,   Int64); \
    XX(HALF_FLOAT,HalfFloat); \
    XX(FLOAT,   Float); \
    XX(DOUBLE,  Double); \
    XX(STRING,  String); \
    XX(BINARY,  Binary); \
    XX(FIXED_SIZE_BINARY,   FixedSizeBinary); \
    XX(DATE32,  Date32); \
    XX(DATE64,  Date64); \
    XX(TIMESTAMP,   Timestamp); \
    XX(TIME32,  Time32); \
    XX(TIME64,  Time64); \
    XX(DECIMAL128,  Decimal128); \
    XX(DECIMAL256,  Decimal256); \
    XX(DURATION,    Duration); \
    XX(LARGE_STRING,    LargeString); \
    XX(LARGE_BINARY,    LargeBinary);

    switch(key_type->id()) {
#define XX(m, b) \
        case arrow::Type::m: \
            key_builder = std::make_shared<arrow::b##Builder>(key_type, arrow::default_memory_pool()); \
            break;
        TYPE_LIST(XX)
#undef XX
        default:
            break;
    }

    switch(value_type->id()) {
#define XX(m, b) \
        case arrow::Type::m: \
            val_builder = std::make_shared<arrow::b##Builder>(value_type, arrow::default_memory_pool()); \
            break;
        TYPE_LIST(XX)
#undef XX
        default:
            break;
    }
    m_builders.push_back(std::make_shared<arrow::MapBuilder>(arrow::default_memory_pool(), key_builder, val_builder, key_sorted));
    return m_fields.size() - 1;
}

std::shared_ptr<arrow::Table> ArrowTableBuilder::finish() {
    std::vector<std::shared_ptr<arrow::Array> > arrs;
    arrs.resize(m_builders.size());
    for(size_t i = 0; i < m_builders.size(); ++i) {
        m_builders[i]->Finish(&arrs[i]);
    }
    auto schema = arrow::schema(m_fields);
    return arrow::Table::Make(schema, arrs);
}

bool WriteParquetFile(const std::shared_ptr<arrow::Table>& table, const std::string& name, int32_t rowgroups) {
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    PARQUET_ASSIGN_OR_THROW(
        outfile,
        arrow::io::FileOutputStream::Open(name));
    PARQUET_THROW_NOT_OK(
        parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, rowgroups));
    return true;
}

}
