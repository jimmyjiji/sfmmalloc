#Custom Malloc Implementation. 

Allows users to allocate custom amounts of memory in bytes through sfmmalloc

###Program also includes implementations of 
1. free
2. realloc
3. calloc 


Memory is managed through an **explicit free list**.
User has the option however to cycle through a LIFO organized list or a Address based list. This allocated splits and coalesces blocks accordingly to headers and footers that are created during allocation. 
Memory is allocated by adjusting the heap using sf_sbrk

**All blocks are 16 bytes aligned**

###Utility Functions
All functions are in sfutil.o

 

This routine will initialize your memory allocator. It should be called * in your implementation ONCE, before using any of the other sfmm functions.
@param max_heap_size Unsigned value determining the maximum size of your heap.


*void sf_sbrk(size_t increment)*


 Function which outputs the state of the free-list to stdout. * Performs checks on the placement of the header and footer, and if the memory payload is correctly aligned. See sf_snapshot section for details on the output format.
 @param verbose If true, snapshot will additionally print out * each memory block using the sf_blockprint function.


*void sf_snapshot(bool verbose)*

Function which prints human readable block format readable format.
@param block Address of the block header in memory. 


*void sf_blockprint(void* block)*


Prints human readable block format from the address of the payload. IE. subtracts header size from the data pointer to obtain the address * of the block header. Calls sf_blockprint internally to print.
@param data Pointer to payload data in memory (value returned by sf_malloc).


*void sf_varprint(void *data)*






