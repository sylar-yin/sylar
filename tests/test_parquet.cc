#include "sylar/sylar.h"
#include "sylar/util/parquet.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_write() {
    sylar::ArrowTableBuilder::ptr builder = std::make_shared<sylar::ArrowTableBuilder>();
    builder->addColumn("id", arrow::int64(), true);
    builder->addColumn("name", arrow::utf8(), true);
    builder->addColumn("sex", arrow::int32(), true);
    builder->addListColumn("int64arr", arrow::int64(), true, true);
    builder->addMapColumn("map_int_str", arrow::int64(), arrow::utf8(), true);

    auto id = builder->getColumnAs<arrow::Int64Builder>("id");
    auto name = builder->getColumnAs<arrow::StringBuilder>("name");
    auto sex = builder->getColumnAs<arrow::Int32Builder>("sex");
    auto int64arr = builder->getColumnAs<arrow::ListBuilder>("int64arr");
    auto map_int_str = builder->getColumnAs<arrow::MapBuilder>("map_int_str");

    for(int i = 0; i < 10; ++i) {
        if(i == 0) {
            id->AppendNull();
        } else {
            id->Append(i + 1000);
        }
        name->Append("name_" + std::to_string(1000 + i));
        switch(i % 3) {
            case 0:
                sex->Append(1);
                break;
            case 1:
                sex->Append(2);
                break;
            default:
                sex->AppendNull();
                break;
        }
        if(i % 3 == 0) {
            int64arr->AppendNull();
            map_int_str->AppendEmptyValue();
        } else if(i % 3 == 1) {
            int64arr->AppendEmptyValue();
            map_int_str->Append();

            auto key = dynamic_cast<arrow::Int64Builder*>(map_int_str->key_builder());
            auto item = dynamic_cast<arrow::StringBuilder*>(map_int_str->item_builder());

            key->Append(i * 1000);
            item->Append("value_" + std::to_string(i * 1000));
            key->Append(i * 1000 + 1);
            item->Append("value_" + std::to_string(i * 1000 + 1));
        } else if(i % 3 == 2) {
            map_int_str->AppendNull();
            //int64arr->AppendEmptyValue();
            //arrow::Int64Builder b;
            //b.AppendValues({i* 100 + 1, i * 100 + 2});
            //std::shared_ptr<arrow::Array> a;
            //b.Finish(&a);
            //int64arr->AppendArraySlice(*a->data(), 0, a->length());
            //int64arr->value_builder();
            int64arr->Append(true);
            auto t = dynamic_cast<arrow::Int64Builder*>(int64arr->value_builder());
            t->Append(i * 100 + 1);
            t->AppendNull();
            t->Append(i * 100 + 3);
        }
    }

    auto tab = builder->finish();
    SYLAR_LOG_INFO(g_logger) << tab->ToString();
    SYLAR_LOG_INFO(g_logger) << tab->schema()->ToString(true);
    auto filename = "test.parquet";
    sylar::WriteParquetFile(tab, filename, 10);

    auto reader = sylar::ParquetFileReader::Open(filename);
    SYLAR_LOG_INFO(g_logger) << reader->toString();
    auto tb = reader->readTable();
    SYLAR_LOG_INFO(g_logger) << "-----";
    SYLAR_LOG_INFO(g_logger) << tb->toString();
    SYLAR_LOG_INFO(g_logger) << "-----";
    auto col = tb->getColumnData(0);
    SYLAR_LOG_INFO(g_logger) << col->getNumChunks();
}

void test_read(int argc, char** argv) {
    if(argc < 2) {
        SYLAR_LOG_INFO(g_logger) << "use as[" << argv[0] << " parquet_file]";
        return;
    }
    try {
        auto reader = sylar::ParquetFileReader::Open(argv[1]);
        SYLAR_LOG_INFO(g_logger) << reader->toString();
    } catch (std::exception& ex) {
        SYLAR_LOG_INFO(g_logger) << ex.what();
    }
}

int main(int argc, char** argv) {
    test_read(argc, argv);
    //test_write();
    return 0;
    auto reader = sylar::ParquetFileReader::Open("df.parquet.gzip");
    auto schema = reader->getSchema();

    SYLAR_LOG_INFO(g_logger) << schema->toString();
    SYLAR_LOG_INFO(g_logger) << reader->toString();

    auto tab = reader->readTable();
    SYLAR_LOG_INFO(g_logger) << tab->toString();

    auto arr = tab->getColumnData(0);
    auto i64arr = arr->chunkAs<arrow::Int64Array>(0);
    for(auto it = i64arr->begin();
            it != i64arr->end(); ++it) {
        std::cout << **it << "\t";
    }
    std::cout << std::endl;

    arr = tab->getColumnData(2);
    //auto i64listarr = arr->chunkAs<arrow::ListArray>(0);
    auto i64listarr = arr->chunkListType(0);
    for(auto i = 0;
            i != i64listarr->getLength(); ++i) {
        if(!i64listarr->isNull(i)) {
            //auto i64 = std::dynamic_pointer_cast<arrow::Int64Array>(i64listarr->value_slice(i));
            auto i64 = i64listarr->getValueAs<arrow::Int64Array>(i);
            std::cout << "[";
            for(auto it = i64->begin();
                    it != i64->end(); ++it) {
                if(*it) {
                    std::cout << **it << "\t";
                } else {
                    std::cout << "null" << "\t";
                }
            }
            std::cout << "]\t";
        } else {
            std::cout << "null" << "\t";
        }
    }
    std::cout << std::endl;
    return 0;
}
