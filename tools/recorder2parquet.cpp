#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"
#include <parquet/arrow/writer.h>
#include <arrow/api.h>
#include <arrow/io/api.h>
/*
 *  * Write all original records (encoded and compressed) to text file
 *   */
void write_to_parquet(const char* path, int rank, Record *records, int len, RecorderReader *reader) {
    std::shared_ptr<arrow::Schema> schema = arrow::schema(
            {arrow::field("rank", arrow::int32()),
             arrow::field("tstart", arrow::float32()), arrow::field("tend", arrow::float32()),
             arrow::field("func_id", arrow::utf8()), arrow::field("res", arrow::int32()),
             arrow::field("arg_count", arrow::int32()), arrow::field("args_1", arrow::utf8()),
             arrow::field("args_2", arrow::utf8()), arrow::field("args_3", arrow::utf8()),
             arrow::field("args_4", arrow::utf8()), arrow::field("args_5", arrow::utf8()),
             arrow::field("args_6", arrow::utf8()), arrow::field("args_7", arrow::utf8()),
             arrow::field("args_8", arrow::utf8()), arrow::field("args_9", arrow::utf8()),
             arrow::field("args_10", arrow::utf8())});
    arrow::Int32Builder rankBuilder, resBuilder, arg_countBuilder;
    arrow::FloatBuilder tstartBuilder, tendBuilder;
    arrow::StringBuilder func_idBuilder, args_Builder[10];
    int i, arg_id;
    for(i = 0; i < len; i++) {
        rankBuilder.Append(rank);
        tstartBuilder.Append(records[i].tstart);
        tendBuilder.Append(records[i].tend);
        func_idBuilder.Append(reader->func_list[records[i].func_id]);
        resBuilder.Append(records[i].res);
        arg_countBuilder.Append(records[i].arg_count);
        for (arg_id = 0; arg_id < 10; arg_id++) {
            if(arg_id < records[i].arg_count){
                args_Builder[arg_id].Append(records[i].args[arg_id]);
            }else{
                args_Builder[arg_id].Append("");
            }
        }
    }
    std::shared_ptr<arrow::Array> rankArray,resArray,arg_countArray, tstartArray, tendArray,
                                  func_idArray, argsArray[10];
    rankBuilder.Finish(&rankArray);
    tstartBuilder.Finish(&tstartArray);
    tendBuilder.Finish(&tendArray);
    func_idBuilder.Finish(&func_idArray);
    resBuilder.Finish(&resArray);
    arg_countBuilder.Finish(&arg_countArray);
    for (arg_id = 0; arg_id < 10; arg_id++) {
    	args_Builder[arg_id].Finish(&argsArray[arg_id]);
    }
    auto table = arrow::Table::Make(schema, {rankArray  , tstartArray  , tendArray  ,
                                func_idArray  , resArray  , arg_countArray  ,
                                argsArray[0], argsArray[1], argsArray[2],
                                argsArray[3], argsArray[4], argsArray[5],
                                argsArray[6], argsArray[7], argsArray[8],
                                argsArray[9] });
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    PARQUET_ASSIGN_OR_THROW(
            outfile,
            arrow::io::FileOutputStream::Open(path));
    int64_t chunk_size = 1024;
    PARQUET_THROW_NOT_OK(
            parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024));
}

int main(int argc, char **argv) {

    char textfile_dir[256], textfile_path[256];
    sprintf(textfile_dir, "%s/_parquet", argv[1]);
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    RecorderReader reader;
    recorder_read_traces(argv[1], &reader);

    int rank;
    for(rank = 0; rank < reader.RGD.total_ranks; rank++) {

        sprintf(textfile_path, "%s/%d.parquet" , textfile_dir, rank);
        Record* records = reader.records[rank];
        write_to_parquet(textfile_path, rank, records, reader.RLDs[rank].total_records, &reader);
    }

    release_resources(&reader);

    return 0;
}

