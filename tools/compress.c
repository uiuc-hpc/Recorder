#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define TICK 0.000001
double START_TIMESTAMP = 0;

typedef struct Record_t {
    int tstart, tdur;
    char *func_id;
    int arg_count;
    char **args;
} Record;


/**
 * Parse one line in text format to a Record type
 * Text format: tstart tend func_name arg1 arg2 arg3 ... argN
 */
Record parse_record(char *args_str) {
    Record record;
    char* tmp        = args_str;
    char* last_comma = 0;
    char delim[] = " ";
    size_t count     = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (delim[0] == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for the last token. */
    count += last_comma < (args_str + strlen(args_str) - 1);
    record.arg_count = count - 3;   // minus tstart, tdur, func_id

    record.args = malloc(sizeof(char*) * count);
    size_t idx  = 0;

    // Record.tstart
    char* token = strtok(args_str, delim);
    if (START_TIMESTAMP == 0) {
        START_TIMESTAMP = atof(token);
    }
    record.tstart = (atof(token) - START_TIMESTAMP) / TICK;
    // Record.tdur
    token = strtok(0, delim);
    record.tdur = atof(token) / TICK;
    // Record.func_id
    token = strtok(0, delim);
    record.func_id = strdup(token);
    // All arguments
    token = strtok(0, delim);
    while (token) {
        *(record.args + idx++) = strdup(token);
        token = strtok(0, delim);
    }

    return record;
}



void write_uncompressed_record(FILE *f, Record record) {
    char status = '0';
    fwrite(&status, sizeof(char), 1, f);
    fwrite(&(record.tstart), sizeof(int), 1, f);
    fwrite(&(record.tdur), sizeof(int), 1, f);
    //fwrite(record.func_id, strlen(record.func_id), 1, f);
    fwrite(record.func_id, sizeof(short), 1, f);
    for(size_t i = 0; i < record.arg_count; i++) {
        fprintf(f, " ");
        if (record.args[i][0] == '/')   // it's a path
            fwrite(record.args[i], 2, 1, f);
        else
            fwrite(record.args[i], strlen(record.args[i]), 1, f);
    }
    fprintf(f, "\n");
}
void write_compressed_record(FILE *f, char ref_id, Record record){
    char status = '0';
    fwrite(&status, sizeof(char), 1, f);
    fwrite(&(record.tstart), sizeof(int), 1, f);
    fwrite(&(record.tdur), sizeof(int), 1, f);
    fwrite(&ref_id, sizeof(char), 1, f);
    for(int i = 0; i < record.arg_count; i++) {
        fprintf(f, " ");
        if (record.args[i][0] == '/')   // it's a path
            fwrite(record.args[i], 2, 1, f);
        else
            fwrite(record.args[i], strlen(record.args[i]), 1, f);
    }
    fprintf(f, "\n");
}

Record get_diff_record(Record old_record, Record new_record) {
    Record diff_record;
    diff_record.arg_count = 999;

    // Same function should normally have the same number of arguments
    if (old_record.arg_count != new_record.arg_count)
        return diff_record;

    // Get the number of different arguments
    int count = 0;
    for(int i = 0; i < old_record.arg_count; i++)
        if(strcmp(old_record.args[i], new_record.args[i]) !=0)
            count++;

    // Record.args store only the different arguments
    diff_record.arg_count = count;
    int idx = 0;
    diff_record.args = malloc(sizeof(char *) * count);
    for(int i = 0; i < old_record.arg_count; i++)
        if(strcmp(old_record.args[i], new_record.args[i]) !=0)
            diff_record.args[idx++] = new_record.args[i];
    return diff_record;
}

void write_record(FILE *f, Record window[3], Record new_record) {
    Record diff_record;
    int compress = 0;
    int min_diff_count = 999;
    char ref_window_id = -1;
    for(int i = 0; i < 3; i++) {
        Record record = window[i];
        if ((strcmp(record.func_id, new_record.func_id)==0) && (new_record.arg_count < 8)) {
            Record tmp_record = get_diff_record(record, new_record);
            if(tmp_record.arg_count < 8 && tmp_record.arg_count < min_diff_count) {
                min_diff_count = tmp_record.arg_count;
                ref_window_id = i;
                compress = 1;
                diff_record = tmp_record;
            }
        }
    }

    if(compress) {
        diff_record.tstart = new_record.tstart;
        diff_record.tdur = new_record.tdur;
        diff_record.func_id = new_record.func_id;
        write_compressed_record(f, ref_window_id, diff_record);
    } else {
        write_uncompressed_record(f, new_record);
    }
}



int main(int argc, char **argv) {
    FILE *file = fopen(argv[1], "r");
    FILE *out = fopen("./1.out", "wb");
    char *line = malloc(sizeof(char) * 255);
    size_t ret, len;

    Record window[3];
    Record record;

    // First line
    if ((ret = getline(&line, &len, file)) != -1) {
        if(line[ret-1] == '\n') line[ret-1] = 0;
        record = parse_record(line);
        write_uncompressed_record(out, record);
        window[0] = record;
        window[1] = record;
        window[2] = record;
    }

    // The rest
    while ((ret = getline(&line, &len, file)) != -1) {
        if(line[ret-1] == '\n') line[ret-1] = 0;
        record = parse_record(line);
        write_record(out, window, record);
        //write_uncompressed_record(out, record);
        window[2] = window[1];
        window[1] = window[0];
        window[0] = record;
    }

    fclose(file);
    fclose(out);
}
