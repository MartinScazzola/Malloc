#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "testlib.h"
#include "malloc.h"

static void
successful_malloc_returns_non_null_pointer(void)
{
	char *var = malloc(100);

	ASSERT_TRUE("TEST 01 - successful malloc returns non null pointer",
	            var != NULL);

	free(var);
}

static void
correct_copied_value(void)
{
	char *test_string = "FISOP malloc is working!";
	char *var = malloc(100);


	strcpy(var, test_string);

	ASSERT_TRUE(
	        "TEST 02 - allocated memory should contain the copied value",
	        strcmp(var, test_string) == 0);

	free(var);
}

static void
correct_amount_of_mallocs(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);
	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 03 - amount of mallocs should be one",
	            stats.mallocs == 1);
}

static void
correct_amount_of_frees(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 04 - amount of frees should be one", stats.frees == 1);
}

static void
correct_amount_of_requested_memory(void)
{
	struct malloc_stats stats;

	char *var = malloc(1000);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("TEST 05 - amount of requested memory should be 1000",
	            stats.requested_memory == 1000);
}

static void
minimum_size_of_requested_memory(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE(
	        "TEST 06 - amount of minimum requested memory should be 256",
	        stats.requested_memory == 256);
}

static void
correct_create_first_region(void)
{
	struct malloc_stats stats;
	char *var = malloc(1000);
	free(var);
	get_stats(&stats);
	ASSERT_TRUE("TEST 07 - amount of regions should be 0",
	            stats.amount_of_regions == 0);
}

static void
correct_splitting_regions_amount(void)
{
	struct malloc_stats stats;
	char *var = malloc(2000);
	free(var);
	char *var2 = malloc(600);
	char *var3 = malloc(500);
	get_stats(&stats);
	ASSERT_TRUE("TEST 08 - amount of regions should be 3",
	            stats.amount_of_regions == 3);
	free(var2);
	free(var3);
}

static void
test_right_coalescing(void)
{
	struct malloc_stats stats;
	char *var = malloc(300);
	char *var2 = malloc(500);
	free(var2);
	free(var);
	get_stats(&stats);

	ASSERT_TRUE("TEST 09 - amount of regions should be 0 after two right "
	            "contiguous regions",
	            stats.amount_of_regions == 0);
}

static void
test_left_coalescing(void)
{
	struct malloc_stats stats;
	char *var = malloc(100);
	char *var2 = malloc(200);

	free(var);
	free(var2);

	get_stats(&stats);
	ASSERT_TRUE("TEST 10 - amount of regions should be 0 after two left "
	            "contiguous regions",
	            stats.amount_of_regions == 0);
}


static void
test_left_and_right_coalescing(void)
{
	struct malloc_stats stats;
	char *var = malloc(100);
	char *var2 = malloc(200);
	char *var3 = malloc(150);

	free(var);
	free(var3);
	free(var2);

	get_stats(&stats);
	ASSERT_TRUE("TEST 11 - amount of regions should be 0 after one left "
	            "and one right contiguous regions",
	            stats.amount_of_regions == 0);
}

static void
correct_block_amount()
{
	struct malloc_stats stats;
	char *var = malloc(16 * 1000);
	char *var2 = malloc(16 * 1000);
	char *var3 = malloc(16 * 1000);
	get_stats(&stats);
	ASSERT_TRUE("TEST 12 - amount of little blocks should be 3",
	            stats.amount_of_little_blocks == 3);
	ASSERT_TRUE("	* amount of regions should be 6",
	            stats.amount_of_regions == 6);
	free(var);
	free(var3);
	free(var2);
}

static void
correct_little_mid_and_large_blocks_amount()
{
	struct malloc_stats stats;
	char *var = malloc(500);
	char *var2 = malloc(1000 * 1000);
	char *var3 = malloc(1000 * 1000 * 10);
	get_stats(&stats);
	ASSERT_TRUE("TEST 13 - amount of little blocks should be 1",
	            stats.amount_of_little_blocks == 1);
	ASSERT_TRUE("	* amount of mid blocks should be 1",
	            stats.amount_of_mid_blocks == 1);
	ASSERT_TRUE("	* amount of large blocks should be 1",
	            stats.amount_of_large_blocks == 1);
	ASSERT_TRUE("	* amount of regions should be 6",
	            stats.amount_of_regions == 6);
	free(var);
	free(var3);
	free(var2);
}


static void
should_create_mid_blocks_after_filled_max_little_blocks()
{
	struct malloc_stats stats;
	char *var = NULL;
	for (int i = 0; i < 26; i++) {
		var = malloc(15 * 1024);
	}

	get_stats(&stats);

	ASSERT_TRUE("TEST 14 - amount of little blocks should be 25",
	            stats.amount_of_little_blocks == 25);
	ASSERT_TRUE("	* amount of mid blocks should be 1",
	            stats.amount_of_mid_blocks == 1);
	free(var);
}

static void
test_unmup_blocks()
{
	struct malloc_stats stats;

	char *var = malloc(1000);
	free(var);

	char *var1 = malloc(17 * 1024);
	free(var1);

	char *var2 = malloc(2 * 1024 * 1024);
	free(var2);

	get_stats(&stats);

	ASSERT_TRUE("TEST 15 - amount of little block should be 0",
	            stats.amount_of_little_blocks == 0);
	ASSERT_TRUE("	* amount of mid block should be 0",
	            stats.amount_of_mid_blocks == 0);
	ASSERT_TRUE("	* amount of large block should be 0",
	            stats.amount_of_large_blocks == 0);
}

static void
test_calloc_all_characters_are_zero()
{
	char *var = calloc(100, sizeof(char));
	bool all_null = true;

	for (int i = 0; i < 100; i++) {
		if (var[i] != '\0') {
			all_null = false;
		}
	}

	ASSERT_TRUE("TEST 16 - all characters should be 0", all_null);
}

static void
test_calloc_all_integers_are_zero()
{
	int *var = calloc(100, sizeof(int));
	bool all_null = true;

	for (int i = 0; i < 100; i++) {
		if (var[i] != 0) {
			all_null = false;
		}
	}

	ASSERT_TRUE("TEST 17 - all integers should be 0", all_null);
}


static void
test_calloc_all_integers_are_assigned_correctly()
{
	int *var = calloc(100, sizeof(int));
	bool assigned_correctly = true;

	for (int i = 0; i < 100; i++) {
		var[i] = i;
	}

	for (int i = 0; i < 100; i++) {
		if (var[i] != i) {
			assigned_correctly = false;
		}
	}
	ASSERT_TRUE("TEST 18 - all integers are assigned correctly.",
	            assigned_correctly);
	free(var);
}

static void
test_correct_realloc_ptr_null()
{
	struct malloc_stats stats;
	char *ptr = NULL;
	char *var = realloc(ptr, 1000);
	get_stats(&stats);
	ASSERT_TRUE("TEST 19 - amount of requested_memory should be 1000",
	            stats.requested_memory == 1000);
	free(var);
}

static void
test_correct_realloc_size_zero()
{
	struct malloc_stats stats;
	char *ptr = malloc(500);
	char *var = realloc(ptr, 0);
	get_stats(&stats);
	ASSERT_TRUE("TEST 20 - amount of requested_memory should be 1",
	            stats.frees == 1);
	ASSERT_TRUE("	* new variable should be NULL", !var);
	free(ptr);
}

static void
test_correct_realloc_in_another_variable()
{
	struct malloc_stats stats;
	char *ptr = malloc(500);
	char *var = realloc(ptr, 1000);
	get_stats(&stats);
	ASSERT_TRUE("TEST 21 - equal content in variables", !strcmp(ptr, var));
	free(ptr);
	free(var);
}

static void
test_correct_realloc_in_smaller_region()
{
	struct malloc_stats stats;
	char *ptr = malloc(1000);
	char *var = realloc(ptr, 200);
	get_stats(&stats);
	ASSERT_TRUE("TEST 22 - should be the same pointers", ptr == var);
	free(ptr);
	free(var);
}

static void
test_correct_realloc_in_bigger_region()
{
	struct malloc_stats stats;
	char *ptr = malloc(1000);
	char *var = realloc(ptr, 2000);
	get_stats(&stats);
	ASSERT_TRUE("TEST 23 - amount of requested memory should be 3000",
	            stats.requested_memory == 3000);
	ASSERT_TRUE("	* amount of 'malloc' should be 2", stats.mallocs == 2);
	ASSERT_TRUE("	* amount of 'free' should be 0", stats.frees == 0);
	free(ptr);
	free(var);
}

static void
test_errno_malloc()
{
// to avoid warnings like warning: argument 1 value ‘18446744073709551606’
// exceeds maximum object size 9223372036854775807
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
	char *var = malloc(-10);
#pragma GCC diagnostic pop

	ASSERT_TRUE("TEST 24 - invalid size of malloc should return null", !var);
	ASSERT_TRUE("	* malloc errno should be ENOMEM", errno == ENOMEM);
}

static void
test_errno_calloc()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
	char *var = calloc(-5, 10);
	char *var2 = calloc(5, -10);
	char *var3 = calloc(-3, -10);
#pragma GCC diagnostic pop

	ASSERT_TRUE("TEST 25 - invalid size of calloc should return null", !var2);
	ASSERT_TRUE("	* calloc errno should be ENOMEM", errno == ENOMEM);
	ASSERT_TRUE("	* invalid number of elements should return null", !var);
	ASSERT_TRUE("	* invalid number of elements & size should return null",
	            !var3);
}

static void
test_errno_realloc()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
	char *var = malloc(5);
	char *var2 = realloc(var, -5);
#pragma GCC diagnostic pop

	ASSERT_TRUE("TEST 26 - invalid size of realloc should return null", !var2);
	ASSERT_TRUE("	* realloc errno should be ENOMEM", errno == ENOMEM);
}


int
main(void)
{
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(correct_copied_value);
	run_test(correct_amount_of_mallocs);
	run_test(correct_amount_of_frees);
	run_test(correct_amount_of_requested_memory);
	run_test(minimum_size_of_requested_memory);
	run_test(correct_create_first_region);
	run_test(correct_splitting_regions_amount);
	run_test(test_right_coalescing);
	run_test(test_left_coalescing);
	run_test(test_left_and_right_coalescing);
	run_test(correct_block_amount);
	run_test(correct_little_mid_and_large_blocks_amount);
	run_test(should_create_mid_blocks_after_filled_max_little_blocks);
	run_test(test_unmup_blocks);
	run_test(test_calloc_all_characters_are_zero);
	run_test(test_calloc_all_integers_are_zero);
	run_test(test_calloc_all_integers_are_assigned_correctly);
	run_test(test_correct_realloc_ptr_null);
	run_test(test_correct_realloc_size_zero);
	run_test(test_correct_realloc_in_another_variable);
	run_test(test_correct_realloc_in_smaller_region);
	run_test(test_correct_realloc_in_bigger_region);
	run_test(test_errno_malloc);
	run_test(test_errno_calloc);
	run_test(test_errno_realloc);

	return 0;
}
