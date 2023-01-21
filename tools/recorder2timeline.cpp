#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"
#include <string>
#include <assert.h>
#include <mpi.h>
#include <ostream>
#include <sstream>
#include <fstream>
#include <iostream>

RecorderReader reader;

struct Writer{
    std::ofstream outFile;
    const char* sep;
    int rank, my_rank, total_ranks;
};

static const char* type_name(int type) {
    switch (type) {
        case RECORDER_POSIX:
            return "POSIX";
        case RECORDER_MPIIO:
            return "MPI I/O";
        case RECORDER_MPI:
            return "MPI";
        case RECORDER_HDF5:
            return "HDF5";
        case RECORDER_FTRACE:
            return "USER";
    }
}

void write_to_json(Record *record, void* arg) {

   int cat = recorder_get_func_type(&reader, record);
   if (record->level == 0 || (cat == 0 || cat == 1 || cat == 3)) {
        Writer *writer = (Writer *) arg;
        bool user_func = (record->func_id == RECORDER_USER_FUNCTION);
        const char *func_name = recorder_get_func_name(&reader, record);

        if (user_func)
            func_name = record->args[0];
        uint64_t ts = uint64_t(record->tstart / reader.metadata.time_resolution);
        int tid = record->tid;
        uint64_t dur = uint64_t((record->tend - record->tstart) / reader.metadata.time_resolution);
        if (dur <= 0) dur = 0;
        std::stringstream ss;
        ss  << writer->sep
            << "{\"pid\":"      << writer->rank
            << ",\"tid\":"      << tid
            << ",\"ts\":"       << ts
            << ",\"name\":\""   << func_name
            << "\",\"cat\":\""  << type_name(cat)
            << "\",\"ph\":\"X\""
            << ",\"dur\":"      << dur
            << ",\"args\":{";
        if (!user_func) {
            ss  << "\"args\":[";
            const char* sep = "";
            for (int arg_id = 0; arg_id < record->arg_count; arg_id++) {
                char *arg = record->args[arg_id];
                ss << sep << "\"" << arg << "\"";
                sep = ",";
            }
            ss  << "],";
        }
        ss << "\"tend\": \"" << record->tend << "\"}}";
        writer->outFile << ss.rdbuf();
        writer->sep = ",\n";
    }
}

int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

int main(int argc, char **argv) {

    if(argc!=2) {
        std::cerr << "Usage: " << argv[0] << " <directory-of-recorder.mt>\n";
        std::exit(1);
    }

    int mpi_size, mpi_rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    char textfile_dir[256];
    sprintf(textfile_dir, "%s/_chrome", argv[1]);

    if(mpi_rank == 0)
        mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    MPI_Barrier(MPI_COMM_WORLD);

    recorder_init_reader(argv[1], &reader);

    // Each rank will process n files (n ranks traces)
    int n = max(reader.metadata.total_ranks/mpi_size, 1);
    int start_rank = n * mpi_rank;
    int end_rank   = min(reader.metadata.total_ranks, n*(mpi_rank+1));
    char textfile_path[256];
    sprintf(textfile_path, "%s/timeline_%d.json", textfile_dir, mpi_rank);
    Writer local;
    local.outFile.open(textfile_path, std::ofstream::trunc|std::ofstream::out);
    local.outFile << "{\"traceEvents\": [\n";
    local.sep = "";
    for(int rank = start_rank; rank < end_rank; rank++) {
        CST cst;
        CFG cfg;
        local.rank = rank;
        recorder_read_cst(&reader, rank, &cst);
        recorder_read_cfg(&reader, rank, &cfg);
        recorder_decode_records(&reader, &cst, &cfg, write_to_json, &local);
        printf("\r[Recorder] rank %d finished, unique call signatures: %d %s\n", rank, cst.entries, textfile_path);
        recorder_free_cst(&cst);
        recorder_free_cfg(&cfg);
    }
    local.outFile << "],\n\"displayTimeUnit\": \"ms\",\"systemTraceEvents\": \"SystemTraceData\",\"otherData\": {\"version\": \"Taxonomy v1.0\" }, \"stackFrames\": {}, \"samples\": []}\n";
    local.outFile.close();
    recorder_free_reader(&reader);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}
