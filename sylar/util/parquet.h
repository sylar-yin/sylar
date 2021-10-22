#ifndef __SYLAR_UTIL_PARQUET_H__
#define __SYLAR_UTIL_PARQUET_H__

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>

namespace sylar {
/*
  enum type {
    /// A NULL type having no physical storage
    NA = 0,

    /// Boolean as 1 bit, LSB bit-packed ordering
    BOOL,

    /// Unsigned 8-bit little-endian integer
    UINT8,

    /// Signed 8-bit little-endian integer
    INT8,

    /// Unsigned 16-bit little-endian integer
    UINT16,

    /// Signed 16-bit little-endian integer
    INT16,

    /// Unsigned 32-bit little-endian integer
    UINT32,

    /// Signed 32-bit little-endian integer
    INT32,

    /// Unsigned 64-bit little-endian integer
    UINT64,

    /// Signed 64-bit little-endian integer
    INT64,

    /// 2-byte floating point value
    HALF_FLOAT,

    /// 4-byte floating point value
    FLOAT,

    /// 8-byte floating point value
    DOUBLE,

    /// UTF8 variable-length string as List<Char>
    STRING,

    /// Variable-length bytes (no guarantee of UTF8-ness)
    BINARY,

    /// Fixed-size binary. Each value occupies the same number of bytes
    FIXED_SIZE_BINARY,

    /// int32_t days since the UNIX epoch
    DATE32,

    /// int64_t milliseconds since the UNIX epoch
    DATE64,

    /// Exact timestamp encoded with int64 since UNIX epoch
    /// Default unit millisecond
    TIMESTAMP,

    /// Time as signed 32-bit integer, representing either seconds or
    /// milliseconds since midnight
    TIME32,

    /// Time as signed 64-bit integer, representing either microseconds or
    /// nanoseconds since midnight
    TIME64,

    /// YEAR_MONTH interval in SQL style
    INTERVAL_MONTHS,

    /// DAY_TIME interval in SQL style
    INTERVAL_DAY_TIME,

    /// Precision- and scale-based decimal type with 128 bits.
    DECIMAL128,

    /// Defined for backward-compatibility.
    DECIMAL = DECIMAL128,

    /// Precision- and scale-based decimal type with 256 bits.
    DECIMAL256,

    /// A list of some logical data type
    LIST,

    /// Struct of logical types
    STRUCT,

    /// Sparse unions of logical types
    SPARSE_UNION,

    /// Dense unions of logical types
    DENSE_UNION,

    /// Dictionary-encoded type, also called "categorical" or "factor"
    /// in other programming languages. Holds the dictionary value
    /// type but not the dictionary itself, which is part of the
    /// ArrayData struct
    DICTIONARY,

    /// Map, a repeated struct logical type
    MAP,

    /// Custom data type, implemented by user
    EXTENSION,

    /// Fixed size list of some logical type
    FIXED_SIZE_LIST,

    /// Measure of elapsed time in either seconds, milliseconds, microseconds
    /// or nanoseconds.
    DURATION,

    /// Like STRING, but with 64-bit offsets
    LARGE_STRING,

    /// Like BINARY, but with 64-bit offsets
    LARGE_BINARY,

    /// Like LIST, but with 64-bit offsets
    LARGE_LIST,

    /// Calendar interval type with three fields.
    INTERVAL_MONTH_DAY_NANO,

    // Leave this at the end
    MAX_ID
  }
*/
class ArrowListArray {
public:
    typedef std::shared_ptr<ArrowListArray> ptr;
    ArrowListArray(std::shared_ptr<arrow::ListArray> data);
    int32_t getLength() const;
    std::shared_ptr<arrow::Array> getValue(int32_t idx) const;
    bool isNull(int32_t idx) const;
    std::shared_ptr<arrow::ListArray> getData() const { return m_data;}

    template<class T>
    std::shared_ptr<T> getValueAs(int32_t idx) const {
        return std::dynamic_pointer_cast<T>(getValue(idx));
    }
private:
std::shared_ptr<arrow::ListArray> m_data;
};

class ArrowChunkedArray {
public:
    typedef std::shared_ptr<ArrowChunkedArray> ptr;
    ArrowChunkedArray(std::shared_ptr<arrow::ChunkedArray> data);

    std::shared_ptr<arrow::ChunkedArray> getData() const { return m_data;}
    std::string toString() const;

    int64_t getLength() const;
    int32_t getNullCount() const;
    int32_t getNumChunks() const;
    ArrowChunkedArray::ptr slice(int64_t offset) const;
    ArrowChunkedArray::ptr slice(int64_t offset, int64_t length) const;

    std::shared_ptr<arrow::Array> chunk(int32_t idx) const;
    template<class T>
    std::shared_ptr<T> chunkAs(int32_t idx) const {
        return std::dynamic_pointer_cast<T>(chunk(idx));
    }

    ArrowListArray::ptr chunkListType(int32_t idx) const;
    //BinaryArray
    //LargeBinaryArray
    //FixedSizeBinaryArray
    //StringArray
    //LargeStringArray
    //ListArray
    //LargeListArray
    //MapArray
    //FixedSizeListArray
    //StructArray
    //Decimal128Array
    //Decimal256Array
    //SparseUnionArray
    //DenseUnionArray
    //BooleanArray
    //Int8Array
    //Int16Array
    //Int32Array
    //Int64Array
    //UInt8Array
    //UInt16Array
    //UInt32Array
    //UInt64Array
    //HalfFloatArray
    //FloatArray
    //DoubleArray
    //Date32Array
    //Date64Array
    //Time32Array
    //Time64Array
    //TimestampArray
    //MonthIntervalArray
    //DayTimeIntervalArray
    //MonthDayNanoIntervalArray
    //DurationArray
    //ExtensionArray
private:
    std::shared_ptr<arrow::ChunkedArray> m_data;
};

class ArrowSchema {
public:
    typedef std::shared_ptr<ArrowSchema> ptr;
    ArrowSchema(std::shared_ptr<arrow::Schema> schema);

    std::shared_ptr<arrow::Field> getField(int32_t idx) const;
    std::shared_ptr<arrow::Field> getFieldByName(const std::string& name) const;
    int32_t getFieldIndex(const std::string& name) const;

    std::shared_ptr<arrow::Schema> getData() const { return m_data;}
    std::string toString() const;
private:
    std::shared_ptr<arrow::Schema> m_data;
};

class ArrowTable {
public:
    typedef std::shared_ptr<ArrowTable> ptr;
    ArrowTable(std::shared_ptr<arrow::Table> tab);
    std::shared_ptr<arrow::Table> getData() const { return m_data;}
    ArrowSchema::ptr getSchema() const;
    std::string toString() const;

    int32_t getRows() const;
    int32_t getCols() const;

    ArrowTable::ptr slice(int64_t offset) const;
    ArrowTable::ptr slice(int64_t offset, int64_t length) const;
    ArrowTable::ptr selectColumns(const std::vector<int32_t>& indices) const;
    ArrowTable::ptr selectColumns(const std::vector<std::string>& cols) const;

    ArrowChunkedArray::ptr getColumnData(int32_t index) const;
    ArrowChunkedArray::ptr getColumnData(const std::string& name) const;
private:
    std::shared_ptr<arrow::Table> m_data;
};

class ParquetFileReader {
public:
    typedef std::shared_ptr<ParquetFileReader> ptr;
    static ParquetFileReader::ptr Open(const std::string& filename);
    parquet::arrow::FileReader* getData() const { return m_data.get();}

    ArrowSchema::ptr getSchema() const;

    ArrowTable::ptr readTable() const;
    ArrowTable::ptr readTable(const std::vector<int32_t>& indices) const;
    ArrowTable::ptr readTable(const std::vector<std::string>& colnames) const;

    std::shared_ptr<parquet::FileMetaData> getMetadata() const;
    std::string toString() const;
private:
    std::unique_ptr<parquet::arrow::FileReader> m_data;
};

//ListBuilder, MapBuilder, Append() to start element
class ArrowTableBuilder {
public:
    typedef std::shared_ptr<ArrowTableBuilder> ptr;
    ArrowTableBuilder();
    int32_t addColumn(const std::string& name, std::shared_ptr<arrow::DataType> type, bool nullable);
    int32_t addListColumn(const std::string& name, std::shared_ptr<arrow::DataType> type, bool list_nullable, bool value_nullable);
    int32_t addMapColumn(const std::string& name, std::shared_ptr<arrow::DataType> type, bool list_nullable, bool value_nullable);
    int32_t addMapColumn(const std::string& name, std::shared_ptr<arrow::DataType> key_type,
                         std::shared_ptr<arrow::DataType> value_type, bool nullable, bool key_sorted = false);
    //int32_t addStructColumn(const std::string& name, const std::vector<std::shared_ptr<arrow::DataType>>& types, bool nullable);
    int32_t getColumnIndex(const std::string& name) const;

    std::shared_ptr<arrow::ArrayBuilder> getColumn(int32_t idx) const;
    std::shared_ptr<arrow::ArrayBuilder> getColumn(const std::string& name) const;

    template<class T>
    std::shared_ptr<T> getColumnAs(int32_t idx) const {
        return std::dynamic_pointer_cast<T>(getColumn(idx));
    }

    template<class T>
    std::shared_ptr<T> getColumnAs(const std::string& name) const {
        return std::dynamic_pointer_cast<T>(getColumn(name));
    }

    std::shared_ptr<arrow::Table> finish();
private:
    std::vector<std::shared_ptr<arrow::Field>> m_fields;
    std::vector<std::shared_ptr<arrow::ArrayBuilder>> m_builders;
    std::map<std::string, int32_t> m_name2indexs;
};

bool WriteParquetFile(const std::shared_ptr<arrow::Table>& table, const std::string& name, int32_t rowgroups);

}

#endif
