#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "/home/jimmy/Desktop/cse320/hwdirectory/hw3/include/sfmm.h"
#include "/home/jimmy/Desktop/cse320/hwdirectory/hw3/include/sfdebug.h"

// Define 20 megabytes as the max heap size
#define MAX_HEAP_SIZE (20 * (1 << 20))
#define VALUE1_VALUE 320
#define VALUE2_VALUE 0xDEADBEEFF00D

#define press_to_cont() do { \
    printf("Press Enter to Continue"); \
    while(getchar() != '\n'); \
    printf("\n"); \
} while(0)

#define null_check(ptr, size) do { \
    if ((ptr) == NULL) { \
        error("Failed to allocate %lu byte(s) for an integer using sf_malloc.\n", (size)); \
        error("Aborting...\n"); \
        assert(false); \
    } else { \
        success("sf_malloc returned a non-null address: %p\n", (ptr)); \
    } \
} while(0)

#define payload_check(ptr) do { \
    if ((unsigned long)(ptr) % 16 != 0) { \
        warn("Returned payload is not divisble by a quadword. %p %% 16 = %lu\n", (ptr), (unsigned long)(ptr) % 16); \
    } \
} while(0)

#define check_prim_contents(actual_value, expected_value, fmt_spec, name) do { \
    if (*(actual_value) != (expected_value)) { \
        error("Expected " name " to be " fmt_spec " but got " fmt_spec "\n", (expected_value), *(actual_value)); \
        error("Aborting...\n"); \
        assert(false); \
    } else { \
        success(name " retained the value of " fmt_spec " after assignment\n", (expected_value)); \
    } \
} while(0)

int main(int argc, char *argv[]) {
    // Initialize the custom allocator
    sf_mem_init(MAX_HEAP_SIZE);

    int* one = sf_malloc(3);
    int*two = sf_malloc(1);
    int *three = sf_malloc(1);
    int* four = sf_malloc(30);
    int* five = sf_malloc(3);
    int* six = sf_malloc(320);
    
    sf_free(six);
    sf_snapshot(true);
        sf_snapshot(true);
    sf_free(one);
        sf_snapshot(true);
    sf_free(two);
        sf_snapshot(true);
    sf_free(three);
        sf_snapshot(true);
    int* seven = sf_malloc(1);
    sf_snapshot(true);
    int* eight = sf_malloc(1);
    sf_snapshot(true);
    int *nine = sf_malloc (1);
    sf_snapshot(true);

    one++;
    two++;
    three++;
    four++;
    five++;
    seven++;
    six++;
    eight++;
    nine++;

    return EXIT_SUCCESS;
}