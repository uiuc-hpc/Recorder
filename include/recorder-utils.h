#ifndef _RECORDER_UTILS_H_
#define _RECORDER_UTILS_H_

#include <fcntl.h>
#include <pthread.h>

void utils_init();
void utils_finalize();
void* recorder_malloc(size_t size);
void recorder_free(void* ptr, size_t size);
pthread_t recorder_gettid(void);
long get_file_size(const char *filename);       // return the size of a file
int accept_filename(const char *filename);      // if include the file in trace
double recorder_wtime(void);                    // return the timestamp
char* itoa(off64_t val);                        // convert an integer to string
char* ftoa(double val);                         // convert a float to string
char* ptoa(const void* ptr);                    // convert a pointer to string
char* arrtoa(size_t arr[], int count);          // convert an array of size_t to a string
char** assemble_args_list(int arg_count, ...);
const char* get_function_name_by_id(int id);
unsigned char get_function_id_by_name(const char* name);
char* realrealpath(const char* path);           // return the absolute path (mapped to id in string)
int mkpath(char* file_path, mode_t mode);       // recursive mkdir()

int min_in_array(int* arr, size_t len);
double recorder_log2(int val);
int recorder_ceil(double val);


#endif
