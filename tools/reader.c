#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "./reader.h"

void read_global_metadata(char* path, RecorderGlobalDef *RGD) {
    FILE* fp = fopen(path, "r+b");
    fread(RGD, sizeof(RecorderGlobalDef), 1, fp);
    fclose(fp);
}

void read_func_list(char* path, RecorderReader *reader) {
    FILE* fp = fopen(path, "r+b");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp) - sizeof(RecorderGlobalDef);
    char buf[fsize];

    fseek(fp, sizeof(RecorderGlobalDef), SEEK_SET); // skip GlobalDef object
    fread(buf, 1, fsize, fp);

    int start_pos = 0, end_pos = 0;
    int func_id = 0;

    for(end_pos = 0; end_pos < fsize; end_pos++) {
        if(buf[end_pos] == '\n') {
            memset(reader->func_list[func_id], 0, sizeof(reader->func_list[func_id]));
            memcpy(reader->func_list[func_id], buf+start_pos, end_pos-start_pos);
            start_pos = end_pos+1;
            func_id++;
        }
    }

    fclose(fp);
}


void read_local_metadata(char* path, RecorderLocalDef *RLD) {
    FILE* fp = fopen(path, "r+b");
    fread(RLD, sizeof(RecorderLocalDef), 1, fp);

    RLD->filemap = (char**) malloc(sizeof(char*) * RLD->num_files);
    RLD->file_sizes = (size_t*) malloc(sizeof(size_t) * RLD->num_files);

    int i;
    for(i = 0; i < RLD->num_files; i++) {
        int id, filename_len;
        fread(&id, sizeof(int), 1, fp);
        fread(&(RLD->file_sizes[i]), sizeof(size_t), 1, fp);
        fread(&filename_len, sizeof(int), 1, fp);

        // do not forget to add the terminating null character.
        RLD->filemap[i] = (char*) malloc(filename_len+1);
        fread(RLD->filemap[i], sizeof(char), filename_len, fp);
        RLD->filemap[i][filename_len] = 0;
    }

    fclose(fp);
}


// Return an array of char*, where each element is an argument
// The input is the original arguments string
char** get_record_arguments(char* str, int arg_count) {

    char** args = (char**) malloc(sizeof(char*) * arg_count);

    int i = 0;
    char* token = strtok(str, " ");

    while( token != NULL ) {
        args[i++] = strdup(token);
        token = strtok(NULL, " ");
    }

    return args;
}


Record* read_records(char* path, RecorderLocalDef* RLD, RecorderGlobalDef *RGD) {

    Record *records = (Record*) malloc(sizeof(Record) * RLD->total_records);

    FILE* fp = fopen(path, "r+b");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *content = (char*)malloc(fsize);
    fread(content, 1, fsize, fp);


    long args_start_pos = 0;
    long rec_start_pos = 0;

    int i, ri = 0;
    while(rec_start_pos < fsize) {
        // read one record
        Record *r = &(records[ri++]);

        // 1. First 14 bytes: status, tstart, tend, func_id, res;
        int tstart; int tend;
        memcpy(&(r->status), content+rec_start_pos+0, 1);
        memcpy(&tstart, content+rec_start_pos+1, 4);
        memcpy(&tend, content+rec_start_pos+5, 4);
        memcpy(&(r->res), content+rec_start_pos+9, 4);
        memcpy(&(r->func_id), content+rec_start_pos+13, 1);

        r->tstart = RLD->start_timestamp + tstart * RGD->time_resolution;
        r->tend = RLD->start_timestamp + tend * RGD->time_resolution;
        r->arg_count = 0;

        // 2. Then arguments splited by ' '
        // '\n' marks the end of one record
        args_start_pos = rec_start_pos + 15;
        for(i = rec_start_pos+14; i < fsize; i++) {
            if(' ' == content[i])
                r->arg_count++;
            if('\n' == content[i]) {
                rec_start_pos = i + 1;
                break;
            }
        }

        if(r->arg_count) {
            int len = rec_start_pos-args_start_pos;
            char* arguments_str = (char*) malloc(sizeof(char) * len);
            memcpy(arguments_str, content+args_start_pos, len-1);
            arguments_str[len-1] = 0;
            r->args = get_record_arguments(arguments_str, r->arg_count);
            free(arguments_str);
        }
    }

    free(content);
    fclose(fp);

    return records;
}

void decompress_records(Record *records, int len) {
    static char diff_bits[] = {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000};

    int i, j;
    for(i = 0; i < len; i++) {
        Record* r = &(records[i]);

        if(r->status) {
            int ref_id = i - 1 - r->func_id;
            Record *ref = &(records[ref_id]);

            r->func_id = ref->func_id;
            r->arg_count = ref->arg_count;
            char **diff_args = r->args;

            // decompress arguments
            int k = 0;
            r->args = (char**) malloc(sizeof(char*) * r->arg_count);
            for(j = 0; j < r->arg_count; j++) {
                if( (j < 7) && (r->status & diff_bits[j]) ) {  // Modify the different ones
                    r->args[j] = diff_args[k++];
                } else {                                        // Copy the same ones
                    r->args[j] = strdup(ref->args[j]);
                }
            }
        }
    }
}

void release_resources(RecorderReader *reader) {
    int ranks = reader->RGD.total_ranks;

    int i, j, rank;
    for (rank = 0; rank < ranks; rank++) {

        Record* records = reader->records[rank];
        for(i = 0; i < reader->RLDs[rank].total_records; i++) {
            for(j = 0; j < records[i].arg_count; j++)
                free(records[i].args[j]);
            free(records[i].args);
        }

        free(records);
    }
    free(reader->records);
    free(reader->RLDs);
}

int compare_by_tstart(const void *lhs, const void *rhs) {
    const Record *r1 = (Record*) lhs;
    const Record *r2 = (Record*) rhs;
    if(r1->tstart > r2->tstart)
        return 1;
    else if(r1->tstart < r2->tstart)
        return -1;
    else
        return 0;
}

void sort_records_by_tstart(Record* records, int num) {
    qsort(records, num, sizeof(Record), compare_by_tstart);
}


void recorder_read_traces(const char* logs_dir, RecorderReader *reader) {

    char global_metadata_file[256];
    char local_metadata_file[256];
    char log_file[256];


    sprintf(global_metadata_file, "%s/recorder.mt", logs_dir);
    read_global_metadata(global_metadata_file, &(reader->RGD));
    read_func_list(global_metadata_file, reader);


    reader->RLDs = (RecorderLocalDef*) malloc(sizeof(RecorderLocalDef) * reader->RGD.total_ranks);
    reader->records = (Record**) malloc(sizeof(Record*) * reader->RGD.total_ranks);

    int rank;
    for(rank = 0; rank < reader->RGD.total_ranks; rank++) {
        sprintf(local_metadata_file, "%s/%d.mt", logs_dir, rank);
        read_local_metadata(local_metadata_file, &(reader->RLDs[rank]));

        sprintf(log_file, "%s/%d.itf", logs_dir, rank);
        reader->records[rank] = read_records(log_file, &(reader->RLDs[rank]), &(reader->RGD));
        decompress_records(reader->records[rank], reader->RLDs[rank].total_records);
        sort_records_by_tstart(reader->records[rank], reader->RLDs[rank].total_records);
        printf("\rRead trace file for rank %d, records: %d", rank, reader->RLDs[rank].total_records);
        fflush(stdout);
    }
    fflush(stdout);
    printf("\rRead traces successfully.                   \n");
    fflush(stdout);
}
