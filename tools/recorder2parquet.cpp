#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"
#include <parquet/stream_writer.h>
#include <parquet/column_writer.h>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <stdint-gcc.h>

using parquet::LogicalType;
using parquet::Repetition;
using parquet::Type;
using parquet::schema::GroupNode;
using parquet::schema::PrimitiveNode;

struct ParquetWriter {
    int rank;
    int index;
    char* path;

    parquet::StreamWriter os;
    std::shared_ptr<arrow::io::FileOutputStream> out_file;
    int num_columns;
    ParquetWriter(int, char*);

    void finish();
};

constexpr int64_t ROW_GROUP_SIZE = 2 * 1024 * 1024;  // 16 MB

std::shared_ptr<GroupNode> SetupSchema() {
    auto fields = parquet::schema::NodeVector();
    fields.push_back(PrimitiveNode::Make("rank", Repetition::REPEATED,
                                         Type::INT32, parquet::ConvertedType::INT_32));
    fields.push_back(PrimitiveNode::Make("thread_id", Repetition::REPEATED,
                                         Type::INT32, parquet::ConvertedType::INT_32));
    fields.push_back(PrimitiveNode::Make("tstart", Repetition::REPEATED,
                                         Type::FLOAT, parquet::ConvertedType::NONE));
    fields.push_back(PrimitiveNode::Make("tend", Repetition::REPEATED,
                                         Type::FLOAT, parquet::ConvertedType::NONE));
    fields.push_back(PrimitiveNode::Make("func_id", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("arg_count", Repetition::REPEATED,
                                         Type::INT32, parquet::ConvertedType::INT_32));
    fields.push_back(PrimitiveNode::Make("args_1", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_2", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_3", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_4", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_5", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_6", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_7", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_8", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_9", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));
    fields.push_back(PrimitiveNode::Make("args_10", Repetition::OPTIONAL,
                                         Type::BYTE_ARRAY, parquet::ConvertedType::UTF8));

    // Create a GroupNode named 'schema' using the primitive nodes defined above
    // This GroupNode is the root node of the schema tree
    return std::static_pointer_cast<GroupNode>(
            GroupNode::Make("schema", Repetition::REQUIRED, fields));
}

ParquetWriter::ParquetWriter(int _rank, char* _path) {
    rank = _rank;
    path = _path;
    auto schema = SetupSchema();
    parquet::WriterProperties::Builder builder;
    std::shared_ptr<parquet::WriterProperties> props = builder.build();
    PARQUET_ASSIGN_OR_THROW(out_file, arrow::io::FileOutputStream::Open(path));
    os = parquet::StreamWriter {
            parquet::ParquetFileWriter::Open(out_file, schema, props)};
    os.SetMaxRowGroupSize(ROW_GROUP_SIZE);
}

void ParquetWriter::finish(void) {
    // Write the bytes to file
    out_file->Close().ok();
}

RecorderReader reader;
void handle_one_record(Record* record, void* arg) {
    ParquetWriter *writer = (ParquetWriter*) arg;
    writer->os << writer->rank;
    writer->os << (int)record->tid;
    writer->os << (float)record->tstart;
    writer->os << (float)record->tend;
    writer->os << recorder_get_func_name(&reader, record->func_id);
    writer->os << (int)record->arg_count;
    for (int arg_id = 0; arg_id < 10; arg_id++) {
        if(arg_id < record->arg_count) {
            writer->os << record->args[arg_id];
        }else{
            writer->os << "";
        }
    }
    writer->os << parquet::EndRow;
    writer->index++;
    if (writer->index % ROW_GROUP_SIZE == 0) {
        writer->os << parquet::EndRowGroup;
    }
}

int main(int argc, char **argv) {

    char parquet_file_dir[256], parquet_file_path[256];
    sprintf(parquet_file_dir, "%s/_parquet", argv[1]);
    mkdir(parquet_file_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    recorder_init_reader(argv[1], &reader);

    for(int rank = 1; rank < reader.metadata.total_ranks; rank++) {

        CST cst;
        CFG cfg;
        recorder_read_cst(&reader, rank, &cst);
        recorder_read_cfg(&reader, rank, &cfg);

        sprintf(parquet_file_path, "%s/%d.parquet" , parquet_file_dir, rank);
        ParquetWriter writer(rank, parquet_file_path);

        recorder_decode_records(&reader, &cst, &cfg, handle_one_record, &writer);

        writer.finish();

        printf("\r[Recorder] rank %d finshed, unique call signatures: %d\n", rank, cst.entries);
        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }

    recorder_free_reader(&reader);
    return 0;
}
