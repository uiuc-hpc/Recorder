#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char** str_split(char* a_str, const char a_delim, size_t *size) {
    char** result    = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    size_t count     = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);
    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);
    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token) {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        *(result + idx) = 0;
    }

    *size = count-1;
    return result;
}

#define TICK 0.000001
double START_TIMESTAMP = 0;

void write_record(FILE *f, int tstart, int tdur, short func_id, char **parameters, size_t param_count) {
    fwrite(&tstart, sizeof(int), 1, f);
    fwrite(&tdur, sizeof(int), 1, f);
    fwrite(&func_id, sizeof(short), 1, f);
    for(size_t i = 2; i < param_count-1; i++) {
        fwrite(parameters[i], strlen(parameters[i]), 1, f);
    }
}

int main(int argc, char **argv) {
    FILE *file = fopen("./sedov_1.itf", "r");
    FILE *out = fopen("./sedov_1.itf.out", "wb");
    char *line = malloc(sizeof(char) * 255);
    size_t ret, len, count;

    char **arr;
    int tstart, tdur;

    // First line
    if ((ret = getline(&line, &len, file)) != -1) {
        arr = str_split(line, ' ', &count);
        START_TIMESTAMP = atof(arr[0]);
        tstart = (atof(arr[0]) - START_TIMESTAMP) / TICK;
        tdur = atof(arr[count-1]) / TICK;
        printf("%d %d\n", tstart, tdur);
    }
    write_record(out, tstart, tdur, 1, arr, count);

    // The rest
    while ((ret = getline(&line, &len, file)) != -1) {
        arr = str_split(line, ' ', &count);
        tstart = (atof(arr[0]) - START_TIMESTAMP) / TICK;
        tdur = atof(arr[count-1]) / TICK;
        write_record(out, tstart, tdur, 1, arr, count);
    }
    fclose(file);
    fclose(out);
}
