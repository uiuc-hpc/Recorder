#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "./reader.h"

void read_global_metadata(char* path, RecorderGlobalDef *RGD) {
    FILE* fp = fopen(path, "r+b");
    fread(RGD, sizeof(RecorderGlobalDef), 1, fp);
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
        args[i++] = token;
        token = strtok(NULL, " ");
    }

    return args;
}


Record* read_records(char* path, int len, RecorderGlobalDef *RGD) {

    Record *records = (Record*) malloc(sizeof(Record) * len);

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

        r->tstart = tstart * RGD->time_resolution;
        r->tend = tend * RGD->time_resolution;
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

        /*
        cout<<i+1<<", status:"<<bitset<8>(r->status)<<", ref-id: "<<bitset<8>(r->func_id)<<", args:";
        if(r->arg_count)
            cout<<r->args[0];
        cout<<endl;
        */

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

/*
int main(int argc, char** argv) {
    const char* logs_dir =  argv[1];

    char global_metadata_file[256];
    char local_metadata_file[256];
    char log_file[256];

    RecorderGlobalDef RGD;
    RecorderLocalDef RLD;
    sprintf(global_metadata_file, "%s/recorder.mt", logs_dir);
    read_global_metadata(global_metadata_file, &RGD);

    for(int rank = 0; rank < RGD.total_ranks; rank++) {
        sprintf(local_metadata_file, "%s/%d.mt", logs_dir, rank);
        read_local_metadata(local_metadata_file, &RLD);

        sprintf(log_file, "%s/%d.itf", logs_dir, rank);
        Record *records = read_records(log_file, RLD.total_records);
        decompress_records(records, RLD.total_records);
        printf("%s: %d\n", log_file,  RLD.total_records);

        free(records);
    }

    return 0;
}
*/
