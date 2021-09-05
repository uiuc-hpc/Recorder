#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "reader.h"

/*
 * Write all original records (encoded and compressed) to text file
 */
/*
void write_to_textfile(const char* path, Record *records, int len, RecorderReader *reader) {
    int i, arg_id;

    FILE* out_file = fopen(path, "w");
    for(i = 0; i < len; i++) {
        Record record = records[i];
        fprintf(out_file, "%f %f %d %s (", record.tstart, record.tend, record.res, reader->func_list[record.func_id]);
        for(arg_id = 0; arg_id < record.arg_count; arg_id++) {
            char *arg = record.args[arg_id];
            fprintf(out_file, " %s", arg);
        }
        fprintf(out_file, " )\n");
    }
    fclose(out_file);
}
*/

int main(int argc, char **argv) {

    char textfile_dir[256], textfile_path[256];
    sprintf(textfile_dir, "%s/_text", argv[1]);
    mkdir(textfile_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    RecorderReader reader;
    recorder_init_reader(argv[1], &reader);

    int entries;
    CallSignature *cst;
    RuleHash *cfg;
    cst = recorder_read_cst(&reader, 0, &entries);
    cfg = recorder_read_cfg(&reader, 0);

    recorder_free_cst(cst, entries);
    recorder_free_cfg(cfg);

    recorder_free_reader(&reader);


    return 0;
}
