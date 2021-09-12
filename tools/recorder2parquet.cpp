#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"
#include <parquet/arrow/writer.h>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <stdint-gcc.h>
#include <mpi.h>

struct ParquetWriter {
    int rank;
    char* base_file;
    int64_t index;
    arrow::Int32Builder rankBuilder, threadBuilder, arg_countBuilder;
    arrow::FloatBuilder tstartBuilder, tendBuilder;
    arrow::StringBuilder func_idBuilder, args_Builder[10];

    std::shared_ptr<arrow::Array> rankArray, threadArray, arg_countArray, tstartArray, tendArray,
            func_idArray, argsArray[10];

    std::shared_ptr<arrow::Schema> schema;
    const int64_t chunk_size = 1024;
    const int64_t NUM_ROWS = 1024*1024*64; // 64 M
    int64_t row_group = 0;
    ParquetWriter(int, char*);

    void finish();
};

ParquetWriter::ParquetWriter(int _rank, char* _path) {
    rank = _rank;
    row_group = 0;

    base_file = _path;
    schema = arrow::schema(
                {arrow::field("rank", arrow::int32()), arrow::field("thread_id", arrow::int32()),
                arrow::field("tstart", arrow::float32()), arrow::field("tend", arrow::float32()),
                arrow::field("func_id", arrow::utf8()),
                arrow::field("arg_count", arrow::int32()), arrow::field("args_1", arrow::utf8()),
                arrow::field("args_2", arrow::utf8()), arrow::field("args_3", arrow::utf8()),
                arrow::field("args_4", arrow::utf8()), arrow::field("args_5", arrow::utf8()),
                arrow::field("args_6", arrow::utf8()), arrow::field("args_7", arrow::utf8()),
                arrow::field("args_8", arrow::utf8()), arrow::field("args_9", arrow::utf8()),
                arrow::field("args_10", arrow::utf8())});
    index = 0;

    rankBuilder = arrow::Int32Builder();
    rankArray.reset();
    threadBuilder = arrow::Int32Builder();
    threadArray.reset();
    arg_countBuilder = arrow::Int32Builder();
    arg_countArray.reset();

    tstartBuilder = arrow::FloatBuilder();
    tstartArray.reset();
    tendBuilder = arrow::FloatBuilder();
    tendArray.reset();


    func_idBuilder = arrow::StringBuilder();
    func_idArray.reset();
    for (int i =0; i< 10; i++){
        args_Builder[i] = arrow::StringBuilder();
        argsArray[i].reset();
    }
}

void ParquetWriter::finish(void) {

    rankBuilder.Finish(&rankArray);
    threadBuilder.Finish(&threadArray);
    tstartBuilder.Finish(&tstartArray);
    tendBuilder.Finish(&tendArray);
    func_idBuilder.Finish(&func_idArray);
    arg_countBuilder.Finish(&arg_countArray);
    for (int arg_id = 0; arg_id < 10; arg_id++) {
        args_Builder[arg_id].Finish(&argsArray[arg_id]);
    }

    auto table = arrow::Table::Make(schema, {rankArray,threadArray, tstartArray, tendArray,
                            func_idArray, arg_countArray  ,
                            argsArray[0], argsArray[1], argsArray[2],
                            argsArray[3], argsArray[4], argsArray[5],
                            argsArray[6], argsArray[7], argsArray[8],
                            argsArray[9] });
    char path[256];
    sprintf(path, "%s_%d.parquet" , base_file, row_group);
    PARQUET_ASSIGN_OR_THROW(auto outfile, arrow::io::FileOutputStream::Open(path));
    PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024));
}

RecorderReader reader;

void handle_one_record(Record* record, void* arg) {
    ParquetWriter *writer = (ParquetWriter*) arg;

    writer->rankBuilder.Append(writer->rank);
    writer->threadBuilder.Append(record->tid);
    writer->tstartBuilder.Append(record->tstart);
    writer->tendBuilder.Append(record->tend);
    bool user_func = (record->func_id == RECORDER_USER_FUNCTION);
    const char* func_name = recorder_get_func_name(&reader, record->func_id);
    if(user_func)
        func_name = record->args[0]; 
    writer->func_idBuilder.Append(func_name);
    writer->arg_countBuilder.Append(record->arg_count);

    for (int arg_id = 0; arg_id < 10; arg_id++) {
        if(arg_id < record->arg_count)
            writer->args_Builder[arg_id].Append(record->args[arg_id]);
        else
            writer->args_Builder[arg_id].Append("");
    }
    writer->index ++;
    if(writer->index % writer->NUM_ROWS == 0) {
        writer->rankBuilder.Finish(&writer->rankArray);
        writer->threadBuilder.Finish(&writer->threadArray);
        writer->tstartBuilder.Finish(&writer->tstartArray);
        writer->tendBuilder.Finish(&writer->tendArray);
        writer->func_idBuilder.Finish(&writer->func_idArray);
        writer->arg_countBuilder.Finish(&writer->arg_countArray);
        for (int arg_id = 0; arg_id < 10; arg_id++) {
            writer->args_Builder[arg_id].Finish(&writer->argsArray[arg_id]);
        }

        auto table = arrow::Table::Make(writer->schema, {writer->rankArray,writer->threadArray, writer->tstartArray, writer->tendArray,
                                                         writer->func_idArray, writer->arg_countArray  ,
                                                         writer->argsArray[0], writer->argsArray[1], writer->argsArray[2],
                                                         writer->argsArray[3], writer->argsArray[4], writer->argsArray[5],
                                                         writer->argsArray[6], writer->argsArray[7], writer->argsArray[8],
                                                         writer->argsArray[9] });
        char path[256];
        sprintf(path, "%s_%d.parquet" , writer->base_file, writer->row_group);
        PARQUET_ASSIGN_OR_THROW(auto outfile, arrow::io::FileOutputStream::Open(path));
        PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024));

        writer->row_group++;
        writer->rankBuilder = arrow::Int32Builder();
        writer->rankArray.reset();
        writer->threadBuilder = arrow::Int32Builder();
        writer->threadArray.reset();
        writer->arg_countBuilder = arrow::Int32Builder();
        writer->arg_countArray.reset();

        writer->tstartBuilder = arrow::FloatBuilder();
        writer->tstartArray.reset();
        writer->tendBuilder = arrow::FloatBuilder();
        writer->tendArray.reset();


        writer->func_idBuilder = arrow::StringBuilder();
        writer->func_idArray.reset();
        for (int i =0; i< 10; i++){
            writer->args_Builder[i] = arrow::StringBuilder();
            writer->argsArray[i].reset();
        }
    }
}

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }


int main(int argc, char **argv) {

    char parquet_file_dir[256], parquet_file_path[256];
    sprintf(parquet_file_dir, "%s/_parquet", argv[1]);
    mkdir(parquet_file_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    int mpi_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    if(mpi_rank == 0)
        mkdir(parquet_file_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    MPI_Barrier(MPI_COMM_WORLD);
    recorder_init_reader(argv[1], &reader);

    // Each rank will process n files (n ranks traces)
    int n = max(reader.metadata.total_ranks/mpi_size, 1);
    int start_rank = n * mpi_rank;
    int end_rank   = min(reader.metadata.total_ranks, n*(mpi_rank+1));

    for(int rank = start_rank; rank < end_rank; rank++) {

        CST cst;
        CFG cfg;
        recorder_read_cst(&reader, rank, &cst);
        recorder_read_cfg(&reader, rank, &cfg);

        sprintf(parquet_file_path, "%s/%d" , parquet_file_dir, rank);
        ParquetWriter writer(rank, parquet_file_path);

        recorder_decode_records(&reader, &cst, &cfg, handle_one_record, &writer);

        writer.finish();

        printf("\r[Recorder] rank %d finished, unique call signatures: %d\n", rank, cst.entries);
        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }

    recorder_free_reader(&reader);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
