#define _GNU_SOURCE
#include <sys/time.h>   // for gettimeofday()
#include <stdarg.h>     // for va_list, va_start and va_end
#include "recorder.h"

bool __recording;

/*
 * Map filename to Integer in binary format
 * The filename must be absolute pathname
 * __filename2id_map is a extern gobal variable defined
 * in recorder.h, initialized in logger.c
 */
hashmap_map* __filename2id_map;
inline char* get_filename_id(const char *filename) {
    int id = -1;
    if (!__recording || !filename || !__filename2id_map || strlen(filename) == 0)
        return itoa(id);

    const char *key = filename;

    /* filename already exists */
    if (hashmap_get(__filename2id_map, key, &id) == MAP_OK)
        return itoa(id);

    /* insert filename into the map */
    id = hashmap_length(__filename2id_map);
    hashmap_put(__filename2id_map, key, id);
    return itoa(id);
}


/*
 * Some of functions are not made by the application
 * And they are operating on wierd files
 * We should not include them in the trace file
 *
 * return 1 if we should ignore the file
 */
inline int exclude_filename(const char *filename) {
    if (filename == NULL) return 0; // pass

    /* these are paths that we will not trace */
    // TODO put these in configuration file?
    static const char *exclusions[] = {"/dev/", "/proc", "/sys", "/etc", "/usr/tce/packages",
                        "pipe:[", "anon_inode:[", "socket:[", NULL};
    int i = 0;
    // Need to make sure both parameters for strncmp are not NULL, otherwise its gonna crash
    while(exclusions[i] != NULL) {
        int find = strncmp(exclusions[i], filename, strlen(exclusions[i]));
        if (find == 0)      // find it. should ignore this filename
            return 1;
        i++;
    }
    return 0;
}

inline long get_file_size(const char *filename) {
    struct stat sb;
    int res = stat(filename, &sb);          // careful here, make sure stat() is not intercepted
    if (res != 0 ) return 0;               // file not exist or some other error

    int is_regular_file = S_ISREG(sb.st_mode);
    if (!is_regular_file) return 0;        // is directory
    return sb.st_size;
}

inline double recorder_wtime(void) {
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + ((double)time.tv_usec / 1000000));
}

/* Integer to stirng */
inline char* itoa(int val) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%d", val);
    return str;
}

/* Integer to stirng */
inline char* ftoa(double val) {
    char *str = malloc(sizeof(char) * 24);
    sprintf(str, "%f", val);
    return str;
}

/* Pointer to string */
inline char* ptoa(const void *ptr) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%p", ptr);
    return str;
}

/* Put many arguments (char *) in a list (char**) */
inline char** assemble_args_list(int arg_count, ...) {
    char** args = malloc(sizeof(char*) * arg_count);
    int i;
    va_list valist;
    va_start(valist, arg_count);
    for(i = 0; i < arg_count; i++)
        args[i] = va_arg(valist, char*);
    va_end(valist);
    return args;
}

/*
 * Convert between function name (char*) and Id (unsigned char)
 * func_list is a fixed string list defined in recorder-log-format.h
 */
inline const char* get_function_name_by_id(int id) {
    if (id < 0 || id > 255) return "WRONG_FUNCTION_ID";
    return func_list[id];
}

unsigned char get_function_id_by_name(const char* name) {
    size_t len = sizeof(func_list) / sizeof(char *);
    unsigned char i;
    for(i = 0; i < len; i++) {
        if (strcmp(func_list[i], name) == 0)
            return i;
    }
    return 255;
}

/*
 * My implementation to replace realpath() system call
 * return the filename id from the hashmap
 */
inline char* realrealpath(const char *path) {
    char *real_pathname = (char*) malloc(PATH_MAX * sizeof(char));
    realpath(path, real_pathname);      // we do not intercept realpath()
    if (real_pathname == NULL)          // realpath() could return NULL on error
        strcpy(real_pathname, path);
    char* id_str = get_filename_id(real_pathname);
    free(real_pathname);
    return id_str;
}

