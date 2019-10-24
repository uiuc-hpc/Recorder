#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define TICK 0.000001
double START_TIMESTAMP = 0;

typedef struct Arguments_t {
    int count;
    char** args;
} Arguments;
typedef struct Record_t {
    int tstart, tdur;
    char *func_id;
    Arguments args;
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
    count = count - 3;  // minus tstart, tdur, func_id

    record.args.args = malloc(sizeof(char*) * count);
    size_t idx  = 0;

    // Record.tstart
    char* token = strtok(args_str, delim);
    if (START_TIMESTAMP == 0) {
        START_TIMESTAMP = atof(token);
    }
    record.tstart = (atof(token) - START_TIMESTAMP) / TICK;
    // Record.tdur
    token = strtok(args_str, delim);
    record.tdur = atof(token) / TICK;
    // Record.func_id
    token = strtok(args_str, delim);
    // All arguments
    while (token) {
        *(record.args.args + idx++) = strdup(token);
        token = strtok(0, delim);
    }

    return record;
}



void write_uncompressed_record(FILE *f, Record record) {
    char status = '0';
    fwrite(&status, sizeof(char), 1, f);
    fwrite(&(record.tstart), sizeof(int), 1, f);
    fwrite(&(record.tdur), sizeof(int), 1, f);
    fwrite(record.func_id, strlen(record.func_id), 1, f);
    for(size_t i = 0; i < record.args.count; i++) {
        fwrite(record.args.args[i], strlen(record.args.args[i]), 1, f);
    }
}
void write_compressed_record(FILE *f, char ref_id, Record record){
    char status = '0';
    fwrite(&status, sizeof(char), 1, f);
    fwrite(&(record.tstart), sizeof(int), 1, f);
    fwrite(&(record.tdur), sizeof(int), 1, f);
    fwrite(&ref_id, sizeof(char), 1, f);
    for(int i = 0; i < record.args.count; i++) {
        fwrite(record.args.args[i], strlen(record.args.args[i]), 1, f);
    }
}

int count_difference(Arguments args1, Arguments args2) {
    int count = 0;
    // Same function should normally have the same number of arguments
    if (args1.count != args2.count)
        return max(args1.count, args2.count);
    for(int i = 0; i < args1.count; i++) {
        if(strcmp(args1.args[i], args2.args[i]) !=0)
            count++;
    }
    return count;
}

void write_record(FILE *f, Record window[3], Record new_record) {
    char *diff[8];
    int compress = 0, min_diff_count = 999;
    char ref_window_id = -1;
    for(int i = 0; i < 3; i++) {
        Record record = window[i];
        if ((strcmp(record.func_id, new_record.func_id)==0) && (new_record.args.count < 8)) {
            int diff_count = count_difference(record.args, new_record.args);
            if(diff_count < 8 && diff_count < min_diff_count) {
                min_diff_count = diff_count;
                ref_window_id = i;
                compress = 1;
            }
        }
    }

    if(compress) {
        write_compressed_record(f, ref_window_id, new_record);
    } else {
        write_uncompressed_record(f, new_record);
    }
}



int main(int argc, char **argv) {
    FILE *file = fopen("./sedov_1.itf", "r");
    FILE *out = fopen("./sedov_1.itf.out", "wb");
    char *line = malloc(sizeof(char) * 255);
    size_t ret, len, count;

    Record window[3];
    Record record;

    // First line
    if ((ret = getline(&line, &len, file)) != -1) {
        record = parse_record(line);
        window[0] = record;
    }
    write_record(out, window, record);

    // The rest
    while ((ret = getline(&line, &len, file)) != -1) {
        record = parse_record(line);
        write_record(out, window, record);
        window[2] = window[1];
        window[1] = window[0];
        window[0] = record;
    }

    fclose(file);
    fclose(out);
}
