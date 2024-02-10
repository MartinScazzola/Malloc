#ifndef _MALLOC_H_
#define _MALLOC_H_

struct malloc_stats {
	int mallocs;
	int frees;
	int requested_memory;
	int amount_of_regions;
	int amount_of_little_blocks;
	int amount_of_mid_blocks;
	int amount_of_large_blocks;
};

void *malloc(size_t size);

void free(void *ptr);

void *calloc(size_t nmemb, size_t size);

void *realloc(void *ptr, size_t size);

void get_stats(struct malloc_stats *stats);

#endif  // _MALLOC_H_
