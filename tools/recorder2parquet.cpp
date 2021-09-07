#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"
#include <parquet/arrow/writer.h>
#include <arrow/api.h>
#include <arrow/io/api.h>

struct ParquetWriter {
    int rank;
    char* path;
    arrow::Int32Builder rankBuilder, arg_countBuilder;
    arrow::FloatBuilder tstartBuilder, tendBuilder;
    arrow::StringBuilder func_idBuilder, args_Builder[10];

    std::shared_ptr<arrow::Schema> schema;

    ParquetWriter(int, char*);

    void finish();
};

ParquetWriter::ParquetWriter(int _rank, char* _path) {
    rank = _rank;
    path = _path;
    schema = arrow::schema(
                {arrow::field("rank", arrow::int32()),
                arrow::field("tstart", arrow::float32()), arrow::field("tend", arrow::float32()),
                arrow::field("func_id", arrow::utf8()),
                arrow::field("arg_count", arrow::int32()), arrow::field("args_1", arrow::utf8()),
                arrow::field("args_2", arrow::utf8()), arrow::field("args_3", arrow::utf8()),
                arrow::field("args_4", arrow::utf8()), arrow::field("args_5", arrow::utf8()),
                arrow::field("args_6", arrow::utf8()), arrow::field("args_7", arrow::utf8()),
                arrow::field("args_8", arrow::utf8()), arrow::field("args_9", arrow::utf8()),
                arrow::field("args_10", arrow::utf8())});
}

void ParquetWriter::finish(void) {
    std::shared_ptr<arrow::Array> rankArray, arg_countArray, tstartArray, tendArray,
                                func_idArray, argsArray[10];
    rankBuilder.Finish(&rankArray);
    tstartBuilder.Finish(&tstartArray);
    tendBuilder.Finish(&tendArray);
    func_idBuilder.Finish(&func_idArray);
    arg_countBuilder.Finish(&arg_countArray);
    for (int arg_id = 0; arg_id < 10; arg_id++) {
        args_Builder[arg_id].Finish(&argsArray[arg_id]);
    }

    auto table = arrow::Table::Make(schema, {rankArray, tstartArray, tendArray,
                            func_idArray, arg_countArray  ,
                            argsArray[0], argsArray[1], argsArray[2],
                            argsArray[3], argsArray[4], argsArray[5],
                            argsArray[6], argsArray[7], argsArray[8],
                            argsArray[9] });
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    PARQUET_ASSIGN_OR_THROW(outfile, arrow::io::FileOutputStream::Open(path));
    int64_t chunk_size = 1024;
    PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024));
}

RecorderReader reader;

void handle_one_record(Record* record, void* arg) {
    ParquetWriter *writer = (ParquetWriter*) arg;

    writer->rankBuilder.Append(writer->rank);
    writer->tstartBuilder.Append(record->tstart);
    writer->tendBuilder.Append(record->tend);
    writer->func_idBuilder.Append(recorder_get_func_name(&reader, record->func_id));
    writer->arg_countBuilder.Append(record->arg_count);

    for (int arg_id = 0; arg_id < 10; arg_id++) {
        if(arg_id < record->arg_count)
            writer->args_Builder[arg_id].Append(record->args[arg_id]);
        else
            writer->args_Builder[arg_id].Append("");
    }
}

int main(int argc, char **argv) {

    char parquet_file_dir[256], parquet_file_path[256];
    sprintf(parquet_file_dir, "%s/_parquet", argv[1]);
    mkdir(parquet_file_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    recorder_init_reader(argv[1], &reader);

    for(int rank = 0; rank < reader.metadata.total_ranks; rank++) {

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

