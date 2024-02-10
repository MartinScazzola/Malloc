#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "malloc.h"
#include "printfmt.h"

#define LITTLE_BLOCK_SIZE 16 * 1024
#define MID_BLOCK_SIZE 1024 * 1024
#define LARGE_BLOCK_SIZE 32 * MID_BLOCK_SIZE
#define MIN_SIZE_REGION 256
#define MAX_LITTLE_BLOCKS 25
#define MAX_MID_BLOCKS 50
#define MAX_LARGE_BLOCKS 25
#define MAGIC_NUMBER 517283971

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

struct block {
	struct block *next;
	struct block *previous;
	struct region *first_region;
};

struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *prev;
	int magic_number;
};

// First block initialization

struct block *little_blocks = NULL;
struct block *mid_blocks = NULL;
struct block *large_blocks = NULL;

struct block *last_little_block = NULL;
struct block *last_mid_block = NULL;
struct block *last_large_block = NULL;


int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
// extern int amount_of_regions = 0; tira warning
int amount_of_regions = 0;
int amount_of_little_blocks = 0;
int amount_of_mid_blocks = 0;
int amount_of_large_blocks = 0;

struct region *
find_region_in_block_best_fit(struct block *block,
                              size_t region_size,
                              size_t block_size)
{
	struct region *best_region = NULL;
	size_t best_reg_dif = block_size - region_size;

	struct block *block_act = block;

	while (block_act) {
		struct region *region = block_act->first_region;
		while (region) {
			if ((region->free) && (region->size >= region_size) &&
			    ((region->size - region_size) < best_reg_dif)) {
				best_reg_dif = region->size - region_size;
				best_region = region;
			}
			region = region->next;
		}
		block_act = block_act->next;
	}
	if (best_region) {
		best_region->free = false;
	}
	return best_region;
}

struct region *
find_region_in_block_first_fit(struct block *block, size_t size)
{
	struct block *block_act = block;
	while (block_act) {
		struct region *region = block_act->first_region;
		while (region) {
			if (region->free && region->size >= size) {
				region->free = false;
				return region;
			}
			region = region->next;
		}
		block_act = block_act->next;
	}
	return NULL;
}

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size
                 __attribute__((unused)))  // find_free_region(size_t size) tira warning
{
	// struct region *region = NULL; tira warning aca
#ifdef FIRST_FIT
	struct region *region = NULL;
	if (size < LITTLE_BLOCK_SIZE) {
		region = find_region_in_block_first_fit(little_blocks, size);
	}
	if (!region && size < MID_BLOCK_SIZE) {
		region = find_region_in_block_first_fit(mid_blocks, size);
	}
	if (!region && size < LARGE_BLOCK_SIZE) {
		region = find_region_in_block_first_fit(large_blocks, size);
	}
	return region;
#endif

#ifdef BEST_FIT
	struct region *region = NULL;
	if (size < LITTLE_BLOCK_SIZE) {
		region = find_region_in_block_best_fit(little_blocks,
		                                       size,
		                                       LITTLE_BLOCK_SIZE);
	}
	if (!region && size < MID_BLOCK_SIZE) {
		region = find_region_in_block_best_fit(mid_blocks,
		                                       size,
		                                       MID_BLOCK_SIZE);
	}
	if (!region && size < LARGE_BLOCK_SIZE) {
		region = find_region_in_block_best_fit(large_blocks,
		                                       size,
		                                       LARGE_BLOCK_SIZE);
	}
	return region;
#endif

	return NULL;
}

void
split_region(struct region *region, size_t size)
{
	struct region *new_region = (void *) region + sizeof(struct region) + size;
	new_region->free = true;
	new_region->size = region->size - size - sizeof(struct region);
	new_region->next = region->next;
	new_region->prev = region;
	new_region->magic_number = MAGIC_NUMBER;
	region->next = new_region;
	region->size = size;

	amount_of_regions++;
}

struct region *
create_region_in_new_block(size_t block_size, struct block **block)
{
	amount_of_regions++;

	struct region *new_region =
	        (void *) *block + sizeof(struct block) + sizeof(struct region);
	new_region->free = false;
	new_region->size =
	        block_size - sizeof(struct block) - sizeof(struct region);
	new_region->next = NULL;
	new_region->prev = NULL;
	new_region->magic_number = MAGIC_NUMBER;

	(*block)->first_region = new_region;

	return new_region;
}
//
struct region *
create_block_with_size(size_t block_size,
                       struct block **block_list,
                       struct block **last_block)
{
	struct block *new_block = mmap(NULL,
	                               block_size,
	                               PROT_WRITE | PROT_READ,
	                               MAP_ANONYMOUS | MAP_PRIVATE,
	                               0,
	                               0);


	if (new_block == MAP_FAILED) {
		perror("ERROR: map failed");
		return NULL;
	}

	new_block->next = NULL;
	if (!*block_list) {
		new_block->previous = NULL;
		*block_list = new_block;
		*last_block = new_block;
	} else {
		new_block->previous = *last_block;
		(*last_block)->next = new_block;
		*last_block = new_block;
	}

	struct region *new_region =
	        create_region_in_new_block(block_size, &new_block);

	return new_region;
}

struct region *
create_block(size_t region_size)
{
	// size_t block_size;
	// struct block *block_list;
	// struct block *last_block; no se usan hay que sacarlas

	if (region_size <= LITTLE_BLOCK_SIZE &&
	    amount_of_little_blocks < MAX_LITTLE_BLOCKS) {
		amount_of_little_blocks++;
		return create_block_with_size(LITTLE_BLOCK_SIZE,
		                              &little_blocks,
		                              &last_little_block);
	} else if (region_size <= MID_BLOCK_SIZE &&
	           amount_of_mid_blocks < MAX_MID_BLOCKS) {
		amount_of_mid_blocks++;
		return create_block_with_size(MID_BLOCK_SIZE,
		                              &mid_blocks,
		                              &last_mid_block);
	} else if (region_size <= LARGE_BLOCK_SIZE &&
	           amount_of_large_blocks < MAX_LARGE_BLOCKS) {
		amount_of_large_blocks++;
		return create_block_with_size(LARGE_BLOCK_SIZE,
		                              &large_blocks,
		                              &last_large_block);
	}
	perror("can't create block too large");
	return NULL;
}

void *
malloc(size_t size)
{
	if ((int) size < 0) {
		errno = ENOMEM;
		return NULL;
	}

	struct region *new_region;

	// set minimum size (256 bytes)
	if (size < MIN_SIZE_REGION) {
		size = MIN_SIZE_REGION;
	}

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	// find available regions
	new_region = find_free_region(size);

	// should be created a new block
	if (!new_region) {
		new_region = create_block(size);
		if (!new_region) {
			errno = ENOMEM;
			return NULL;
		}
	}

	// verify splitting
	if (new_region->size - size >= sizeof(struct region) + MIN_SIZE_REGION) {
		split_region(new_region, size);
	}

	return REGION2PTR(new_region);
}

void
update_block_list(struct block *new_block_list, size_t block_size)
{
	switch (block_size + sizeof(struct block) + sizeof(struct region)) {
	case LITTLE_BLOCK_SIZE:
		little_blocks = new_block_list;
		break;
	case MID_BLOCK_SIZE:
		mid_blocks = new_block_list;
		break;
	case LARGE_BLOCK_SIZE:
		large_blocks = new_block_list;
		break;
	}
}


void
delete_block(struct block *block, size_t size)
{
	// unique block in list
	if (!block->previous && !block->next) {
		update_block_list(NULL, size);

		// block in middle of list
	} else if (block->previous && block->next) {
		block->previous->next = block->next;
		block->next->previous = block->previous;

		// first block
	} else if (!block->previous && block->next) {
		block->next->previous = NULL;
		update_block_list(block->next, size);

		// last block
	} else if (block->previous && !block->next) {
		block->previous->next = block->next;
	}

	// testing
	amount_of_regions--;
	switch (size + sizeof(struct block) + sizeof(struct region)) {
	case LITTLE_BLOCK_SIZE:
		amount_of_little_blocks--;
		break;
	case MID_BLOCK_SIZE:
		amount_of_mid_blocks--;
		break;
	case LARGE_BLOCK_SIZE:
		amount_of_large_blocks--;
		break;
	}
	munmap(block, size + sizeof(struct block) + sizeof(struct region));
}


void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	int *magic_number = (ptr - sizeof(int *));

	if (*magic_number != MAGIC_NUMBER) {
		return;
	}

	struct region *curr = PTR2REGION(ptr);

	assert(curr->free == 0);
	curr->free = true;

	// check if next region is free
	struct region *next = curr->next;

	if (next && next->free) {
		curr->size = next->size + curr->size + sizeof(struct region);
		curr->next = next->next;
		amount_of_regions--;
	}
	// check if previous region is free
	struct region *prev = curr->prev;

	if (prev && prev->free) {
		prev->size = prev->size + curr->size + sizeof(struct region);
		prev->next = curr->next;
		amount_of_regions--;

		if (!prev->prev && !prev->next) {
			delete_block((struct block *) ((void *) prev -
			                               sizeof(struct block)),
			             prev->size);
		}

	} else if (!curr->prev && !curr->next) {
		delete_block((struct block *) ((void *) curr -
		                               sizeof(struct block)),
		             curr->size);
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if ((int) nmemb < 0 || (int) size < 0) {
		errno = ENOMEM;
		return NULL;
	}
	void *ptr = malloc(nmemb * size);
	if (ptr) {
		memset(ptr, 0, nmemb * size);
	} else {
		errno = ENOMEM;
	}
	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	if ((int) size < 0) {
		errno = ENOMEM;
		return NULL;
	}

	if (!ptr && size != 0) {
		return malloc(size);
	} else if (ptr && size != 0) {
		struct region *curr = PTR2REGION(ptr);
		if (curr->size > size) {
			if (curr->size - size >=
			    sizeof(struct region) + MIN_SIZE_REGION) {
				split_region(curr, size);
			}

			return REGION2PTR(curr);
		} else {
			void *new_ptr = malloc(size);
			memcpy(new_ptr, ptr, curr->size);
			return new_ptr;
		}
	}
	free(ptr);
	return NULL;
}

void
get_stats(struct malloc_stats *stats)
{
	stats->mallocs = amount_of_mallocs;
	stats->frees = amount_of_frees;
	stats->requested_memory = requested_memory;
	stats->amount_of_regions = amount_of_regions;
	stats->amount_of_little_blocks = amount_of_little_blocks;
	stats->amount_of_mid_blocks = amount_of_mid_blocks;
	stats->amount_of_large_blocks = amount_of_large_blocks;
}
