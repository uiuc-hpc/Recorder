#define _GNU_SOURCE
#include <sys/time.h>
#include "recorder.h"


/*
 * Map filename to Integer in binary format
 * The filename must be absolute pathname
 * __filename2id_map is a gobal variable defined
 * in recorder.h, initialized in logger.c
 */
hashmap_map* __filename2id_map;
inline int get_filename_id(const char *filename) {
    int id = -1;
    if (!filename || !__filename2id_map || strlen(filename) == 0)
        return id;

    const char *key = filename;

    /* filename already exists */
    if (hashmap_get(__filename2id_map, key, &id) == MAP_OK)
        return id;

    /* insert filename into the map */
    id = hashmap_length(__filename2id_map);
    hashmap_put(__filename2id_map, key, id);
    return id;
}


/*
 * Some of functions are not made by the application
 * And they are operating on wierd files
 * We should not include them in the trace file
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
    int res = stat(filename, &sb);          // careful here, need to be sure stat() is not intercepted
    if (res != 0 ) return -1;               // file not exist or some other error

    int is_regular_file = S_ISREG(sb.st_mode);
    if (!is_regular_file) return -1;        // is directory
    return sb.st_size;
}

double recorder_wtime(void) {
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

/* Pointer to string */
inline char* ptoa(const void *ptr) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%p", ptr);
    return str;
}

