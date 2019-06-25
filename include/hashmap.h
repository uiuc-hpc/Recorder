#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_SIZE (256)
#define MAX_CHAIN_LENGTH (8)

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

/* We need to keep keys and values */
typedef struct _hashmap_element{
	char* key;
	int in_use;
	int data;
} hashmap_element;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _hashmap_map {
	int table_size;
	int size;
	hashmap_element *data;
} hashmap_map;


hashmap_map* hashmap_new();
void hashmap_free(hashmap_map* m);
int hashmap_length(hashmap_map* in);
int hashmap_put(hashmap_map *m, char* key, int value);
int hashmap_get(hashmap_map *m, char* key, int *arg);
int hashmap_remove(hashmap_map *in, char* key);

