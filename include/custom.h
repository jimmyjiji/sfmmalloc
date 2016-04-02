#ifndef CUSTOM
#define CUSTOM
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "sfmm.h"

extern bool firstmalloc;
extern char* endofmalloc;
extern sf_free_header* nextcursor;

sf_header* alloc (sf_header* pointer_header, size_t size);
uint64_t blocksizecalculator(size_t size);

bool valid_address(void* ptr);

#endif
