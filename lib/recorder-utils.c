#define _GNU_SOURCE
#include <sys/time.h>   // for gettimeofday()
#include <stdarg.h>     // for va_list, va_start and va_end
#include "recorder.h"


/*
 * External global values defined in recorder.h
 */
bool __recording;
FilenameHashTable* __filename_hashtable;

// Log pointer addresses in the trace file?
static bool log_pointer = false;
static size_t memory_usage = 0;

void util_init() {
    log_pointer = false;
    const char* s = getenv("RECORDER_LOG_POINTER");
    if(s)
        log_pointer = atoi(s);
}

void utils_finalize() {
    printf("memory usage: %ld\n", memory_usage);
}

void* recorder_malloc(size_t size) {
    if(size == 0)
        return NULL;

    memory_usage += size;
    //if(__recording)
    //    printf("malloc: %ld, now: %ld\n", size, memory_usage);
    return malloc(size);
}
void recorder_free(void* ptr, size_t size) {
    if(size == 0 || ptr == NULL)
        return;
    memory_usage -= size;
    //if(__recording)
    //    printf("free: %ld, now: %ld\n", size, memory_usage);
    free(ptr);
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
inline char* itoa(size_t val) {
    char *str = calloc(16, sizeof(char));
    sprintf(str, "%ld", val);
    return str;
}

/* Integer to stirng */
inline char* ftoa(double val) {
    char *str = calloc(24, sizeof(char));
    sprintf(str, "%f", val);
    return str;
}

/* Pointer to string */
inline char* ptoa(const void *ptr) {
    char* str;
    if(log_pointer) {
        str = calloc(16, sizeof(char));
        sprintf(str, "%p", ptr);
    } else {
        str = calloc(3, sizeof(char));
        str[0] = '%'; str[1] = 'p';
    }
    return str;
}

inline char* arrtoa(size_t arr[], int count) {
    char *str = calloc(16 * count, sizeof(char));
    str[0] = '[';
    int pos = 1;

    int i;
    for(i = 0; i < count; i++) {
        char *s = itoa(arr[i]);
        memcpy(str+pos, s, strlen(s));
        pos += strlen(s);

        if(i == count - 1)
            str[pos++] = ']';
        else
            str[pos++] = ',';

        free(s);
    }
    return str;
}


/* Put many arguments (char *) in a list (char**) */
inline char** assemble_args_list(int arg_count, ...) {
    char** args = recorder_malloc(sizeof(char*) * arg_count);
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
    size_t count = sizeof(func_list) / sizeof(char *);
    if (id < 0 || id >= count ) {
        printf("[Recorder ERROR] Wrong function id: %d\n", id);
        return NULL;
    }
    return func_list[id];
}

unsigned char get_function_id_by_name(const char* name) {
    size_t len = sizeof(func_list) / sizeof(char *);
    unsigned char i;
    for(i = 0; i < len; i++) {
        if (strcmp(func_list[i], name) == 0)
            return i;
    }
    printf("[Recorder ERROR] Missing function %s\n", name);
    return 255;
}

/*
 * My implementation to replace realpath() system call
 * return the filename id from the hashmap
 */
inline char* realrealpath(const char *path) {
    if(!__recording) return strdup(path);

    FilenameHashTable *entry = malloc(sizeof(FilenameHashTable));

    char* res = realpath(path, entry->name);    // we do not intercept realpath()
    if (res == NULL)                            // realpath() could return NULL on error
        strcpy(entry->name, path);

    FilenameHashTable *found;
    HASH_FIND_STR(__filename_hashtable, entry->name, found);

    // return duplicated name, because we need to free record.args later
    if(found) {
        free(entry);
        return strdup(found->name);
    } else {
        // insert to hashtable
        HASH_ADD_STR(__filename_hashtable, name, entry);
        return strdup(entry->name);
    }
}

