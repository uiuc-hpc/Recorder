#define _GNU_SOURCE
#include <unistd.h>
#include <sys/time.h>   // for gettimeofday()
#include <stdarg.h>     // for va_list, va_start and va_end
#include <sys/syscall.h> // for SYS_gettid
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <zlib.h>
#include "recorder.h"

// Log pointer addresses in the trace file?
static bool   log_pointer = false;
static size_t memory_usage = 0;


char** inclusion_prefix;
char** exclusion_prefix;

/**
 * Similar to python str.split(delim)
 * This returns a list of tokens splited by delim
 * need to free the memory after use.
 *
 * This function can not handle two delim in a row.
 * e.g., AB\n\nCC
 */
char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

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

    /* Allocate an extra one for NULL pointer
     * so the caller knows the end of the list */
    count += 1;
    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        while (token) {
            assert(idx < count);
            result[idx++] = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        result[idx] = NULL;
    }
    return result;
}

char** read_prefix_list(const char* path) {
    GOTCHA_SET_REAL_CALL(fopen,  RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(fseek,  RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(ftell,  RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(fread,  RECORDER_POSIX_TRACING);
    GOTCHA_SET_REAL_CALL(fclose, RECORDER_POSIX_TRACING);

    FILE* f = GOTCHA_REAL_CALL(fopen)(path, "r");
    if (f == NULL) {
        RECORDER_LOGERR("[Recorder] invalid prefix file: %s\n", path);
        return NULL;
    }

    GOTCHA_REAL_CALL(fseek)(f, 0, SEEK_END);
    size_t fsize = GOTCHA_REAL_CALL(ftell)(f);
    GOTCHA_REAL_CALL(fseek)(f, 0, SEEK_SET);
    char* data = recorder_malloc(fsize+1);
    data[fsize] = 0;

    GOTCHA_REAL_CALL(fread)(data, 1, fsize, f);
    GOTCHA_REAL_CALL(fclose)(f);

    char** res = str_split(data, '\n');
    recorder_free(data, fsize);

    return res;
}

void utils_init() {
    log_pointer = false;
    const char* s = getenv(RECORDER_STORE_POINTER);
    if(s)
        log_pointer = atoi(s);

    exclusion_prefix = NULL;
    inclusion_prefix = NULL;

    const char *exclusion_fname = getenv(RECORDER_EXCLUSION_FILE);
    if(exclusion_fname)
        exclusion_prefix = read_prefix_list(exclusion_fname);

    const char *inclusion_fname = getenv(RECORDER_INCLUSION_FILE);
    if(inclusion_fname)
        inclusion_prefix = read_prefix_list(inclusion_fname);
}


void utils_finalize() {
    if(inclusion_prefix) {
        for (int i = 0; inclusion_prefix[i] != NULL; i++)
            free(inclusion_prefix[i]);
        free(inclusion_prefix);
    }
    if(exclusion_prefix) {
        for (int i = 0; exclusion_prefix[i] != NULL; i++)
            free(exclusion_prefix[i]);
        free(exclusion_prefix);
    }
}


void* recorder_malloc(size_t size) {
    if(size == 0)
        return NULL;

    memory_usage += size;
    return malloc(size);
}
void recorder_free(void* ptr, size_t size) {
    if(size == 0 || ptr == NULL)
        return;
    memory_usage -= size;

    free(ptr);
    ptr = NULL;
}

/*
 * Some of functions are not made by the application
 * And they are operating on many strange-name files
 *
 * return 1 if we the filename exists in the inclusion list
 * and not exists in the exclusion list.
 */
inline int accept_filename(const char *filename) {
    if (filename == NULL) return 0;

    if(exclusion_prefix) {
        for (int i = 0; exclusion_prefix[i] != NULL; i++) {
            char* prefix = exclusion_prefix[i];
            if ( 0 == strncmp(prefix, filename, strlen(prefix)) )
                return 0;
        }
    }

    if(inclusion_prefix) {
        for (int i = 0; inclusion_prefix[i] != NULL; i++) {
            char* prefix = inclusion_prefix[i];
            if ( 0 == strncmp(prefix, filename, strlen(prefix)) )
                return 1;
        }
        return 0;
    }

    return 1;
}

inline pthread_t recorder_gettid(void)
{
#ifdef SYS_gettid
    return syscall(SYS_gettid);
#else
    return pthread_self();
#endif
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
  // Cannot use PMPI_Wtime here as MPI_Init may not be initialized
  //return PMPI_Wtime();
}

/* Integer to stirng */
inline char* itoa(off64_t val) {
    char *str = calloc(32, sizeof(char));
    sprintf(str, "%ld", val);
    return str;
}

/* float to stirng */
inline char* ftoa(double val) {
    char *str = calloc(32, sizeof(char));
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
    if (id == RECORDER_USER_FUNCTION)
        return "user_function";

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
    RECORDER_LOGERR("[Recorder] error: missing function %s\n", name);
    return 255;
}

/*
 * My implementation to replace realpath() system call
 */
inline char* realrealpath(const char *path) {
    char* res = realpath(path, NULL);   // we do not intercept realpath()

    // realpath() could return NULL on error 
	// e.g., when the file not exists
    if (res == NULL) {
		if(path[0] == '/') return strdup(path);
		char cwd[512] = {0};
		GOTCHA_SET_REAL_CALL(getcwd, RECORDER_POSIX_TRACING);
		char* tmp = GOTCHA_REAL_CALL(getcwd)(cwd, 512);
        if (tmp == NULL) {
            RECORDER_LOGERR("[Recorder] error: getcwd failed\n");
            return NULL;
        }

		res = malloc(strlen(cwd) + strlen(path) + 20);
		sprintf(res, "%s/%s", cwd, path);
	}
    return res;
}

/**
 * Like mkdir() but also create parent directory
 * if not exists
 */
int mkpath(char* file_path, mode_t mode) {

    GOTCHA_SET_REAL_CALL(mkdir, RECORDER_POSIX_TRACING);

    assert(file_path && *file_path);

    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (GOTCHA_REAL_CALL(mkdir)(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}

int min_in_array(int* arr, size_t len) {
    int min_val = arr[0];
    for(int i = 1; i < len; i++) {
        if(arr[i] < min_val)
            min_val = arr[i];
    }
    return min_val;
}


double recorder_log2(int val) {
    return log(val)/log(2);
}


int recorder_ceil(double val) {
    int tmp = (int) val;
    if(val > tmp)
        return tmp + 1;
    else
        return tmp;
}


void recorder_write_zlib(unsigned char* buf, size_t buf_size, FILE* out_file) {
    GOTCHA_SET_REAL_CALL(fwrite, RECORDER_POSIX_TRACING);
    int ret;
    unsigned have;
    z_stream strm;

    unsigned char out[buf_size];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    // ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        RECORDER_LOGERR("[Recorder] fatal error: can't initialize zlib.");
        return;
    }

    strm.avail_in = buf_size;
    strm.next_in  = buf;
    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
        strm.avail_out = buf_size;
        strm.next_out = out;
        ret = deflate(&strm, Z_FINISH);    /* no bad return value */
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        have = buf_size - strm.avail_out;
        if (GOTCHA_REAL_CALL(fwrite)(out, 1, have, out_file) != have) {
            RECORDER_LOGERR("[Recorder] fatal error: zlib write out error.");
            (void)deflateEnd(&strm);
            return;
        }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);         /* all input will be used */

    /* clean up and return */
    (void)deflateEnd(&strm);
}
