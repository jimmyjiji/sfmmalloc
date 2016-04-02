/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "sfmm.h"
#include "custom.h"

/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
sf_free_header* freelist_head = NULL;
sf_free_header* nextcursor = NULL;

bool firstmalloc = true;
const void* bottomofheap;
char* endofmalloc;
bool samesize = false;
#define PAGE_SIZE 4096

#ifndef LIFO
	#ifdef ADDRESS
		#define LIFO false
	#else
		#define LIFO true
	#endif
#endif

#ifndef FIRST
	#ifdef NEXT
		#define FIRST false
	#else
		#define FIRST true 
	#endif
#endif

void* sf_malloc(size_t size) {
	if (firstmalloc) {
		bottomofheap = sf_sbrk(0)+8;
		firstmalloc = false;
	}
	if (size == 0 || size > 4294967296 ){
		return NULL;
		errno = ENOMEM;
	}
	if (freelist_head == NULL) {
		sf_header* pointer_header = (sf_sbrk(0))+8;
		char* freeblockcreator = (char*) pointer_header;
		endofmalloc = (char*)pointer_header;
		unsigned int ACTUALPAGESIZE;
		if (size < PAGE_SIZE) {
			if (sf_sbrk(PAGE_SIZE) == (void*)-1){
				return NULL;
				errno = ENOMEM;
			}
			else
				endofmalloc += PAGE_SIZE;
			ACTUALPAGESIZE = PAGE_SIZE;
		} else {
			ACTUALPAGESIZE = PAGE_SIZE*((size/PAGE_SIZE)+1);
			if (sf_sbrk(ACTUALPAGESIZE) == (void*)-1) {
				return NULL;
				errno = ENOMEM;
			}
			else
				endofmalloc += (ACTUALPAGESIZE);
		}

		pointer_header = alloc(pointer_header, size)+1;
		uint64_t actualblocksize = blocksizecalculator(size)*16;

		freeblockcreator += actualblocksize	;
		sf_free_header* freeblock = (sf_free_header*) freeblockcreator;
		freeblock -> header.alloc = 0;
		freeblock -> header.requested_size = ACTUALPAGESIZE - actualblocksize;
		freeblock -> header.block_size = (ACTUALPAGESIZE/16) - blocksizecalculator(size);
		freeblock -> next = 0;
		freeblock -> prev = 0;
		freeblockcreator += freeblock -> header.block_size * 16 -8;

		sf_footer* freeblockfooter = (sf_footer*) freeblockcreator;
		freeblockfooter -> alloc = 0;
		freeblockfooter -> block_size = freeblock -> header.block_size;

		freelist_head = freeblock;

		return pointer_header;
	} else {
		sf_free_header* freecursor;
		sf_header* pointer_header;

		if (FIRST) 
			freecursor = freelist_head;
		else {
			if (nextcursor == NULL || nextcursor -> next == 0)
				nextcursor = freelist_head;
			freecursor = nextcursor;
		}

		//loop through the free list 
		while ((int)freecursor -> header.block_size - (int)blocksizecalculator(size) < 2 && freecursor -> next != 0) {
			if (freecursor -> header.block_size == blocksizecalculator(size)){	
				samesize = true;		
				break;
			}
			freecursor = freecursor -> next;
			if (!FIRST)
				nextcursor = nextcursor -> next;
		}

		if (samesize) {
			if (freecursor == freelist_head) {
				freelist_head = freecursor -> next;
				freelist_head -> prev = 0;
			} else {
				if (freecursor -> prev != 0)
					freecursor -> prev -> next = freecursor -> next;
				if (freecursor -> next != 0)
					freecursor -> next -> prev = freecursor -> prev;
				else if (freecursor -> next == 0) {
					freecursor -> prev -> next = 0;
				}
			}


			samesize = false;
			pointer_header = (sf_header*) freecursor;
			return alloc(pointer_header, size) +1;
		}
		//if freelist has reached the end and theres no space left 
		if (freecursor -> next == 0 && (int)freecursor -> header.block_size - (int)blocksizecalculator(size) < 2) {
			pointer_header = sf_sbrk(0)+8;
			unsigned int ACTUALPAGESIZE;
			if (size < PAGE_SIZE) {
				if (sf_sbrk(PAGE_SIZE) == (void*)-1) {
					return NULL;
					errno = ENOMEM;
				}
				else
					endofmalloc += PAGE_SIZE;
				ACTUALPAGESIZE = PAGE_SIZE;
			} else {
				ACTUALPAGESIZE = PAGE_SIZE*((size/PAGE_SIZE)+1);
				if (sf_sbrk(ACTUALPAGESIZE) == (void*)-1) {
					return NULL;
					errno = ENOMEM;
				}
				else
					endofmalloc += (ACTUALPAGESIZE);
			}
		} else {
			char* splitcursor = (char*)freecursor;
			splitcursor += blocksizecalculator(size)* 16;
			sf_free_header* splitter = (sf_free_header*) splitcursor;
			splitter -> header.block_size = freecursor -> header.block_size - blocksizecalculator(size);
			splitter -> header.requested_size = (splitter -> header.block_size * 16) - 32;
			splitter -> header.alloc = 0;

			splitter -> next = freecursor -> next;
			splitter -> prev = freecursor -> prev;
			splitcursor += splitter->header.block_size * 16 -8;

			sf_footer* splitfooter = (sf_footer*)splitcursor;
			splitfooter -> block_size = splitter -> header.block_size;
			splitfooter -> alloc = 0;

			if (freecursor == freelist_head) {
				freelist_head = splitter;
				if (freecursor -> next != 0)
					freecursor -> next -> prev = splitter;
				if (!FIRST)
					nextcursor = splitter;
			} else {
				splitter -> prev = freecursor -> prev;
				splitter -> next = freecursor -> next;
				if (freecursor -> prev != 0)
					freecursor -> prev -> next = splitter;
				if (freecursor -> next != 0)
					freecursor -> next -> prev = splitter;
			}
			pointer_header = (sf_header*) freecursor;

		}
		return alloc(pointer_header, size)+1;
	}

}	

void sf_free(void *ptr) {
	//all headers and footers adjacent to this memory block 
	sf_header* currentheader;
	sf_footer* currentfooter;
	sf_free_header* freecurrentheader;
	sf_header* prevheader;
	sf_footer* prevfooter;
	sf_header* nextheader;

	if (ptr == NULL || !valid_address(ptr)) {
		perror("Invalid Address!");
		errno = EINVAL;
		return;
	}
	//clear alloc
	currentheader = (sf_header*)ptr; 
	char* ccurrentfooter = (char*) currentheader;
	currentheader--;
	currentheader -> alloc = 0;
	freecurrentheader = (sf_free_header*)currentheader;
	freecurrentheader -> header = *currentheader;
	freecurrentheader -> next = 0;
	freecurrentheader -> prev= 0;

	ccurrentfooter += (currentheader -> block_size -1) * 16;
	currentfooter = (sf_footer*) ccurrentfooter;
	currentfooter -> alloc = 0;
	


	//find prev footer and header

	char* cprev = (char*) currentheader;
	prevfooter = (sf_footer*)(cprev-8);
	prevheader = (sf_header*) cprev;

	//there is nothing behind this block 
	if (prevfooter -> block_size != 0 && (void*)cprev >= bottomofheap) {
		cprev -= prevfooter -> block_size * 16;
		prevheader = (sf_header*)cprev;
	} else {
		prevheader = NULL;
		prevfooter = NULL;
	}


	//find next header and footer
	char* cnextheader = (char *)currentheader;
	nextheader = (sf_header*)(ccurrentfooter + 8);
	//nothing after this block
	if (nextheader -> block_size != 0 && (char*)cnextheader < endofmalloc) {
			cnextheader = (char*)nextheader;
			cnextheader += (nextheader -> block_size)*16 - 8;
	} else {
		nextheader = NULL;
	}

	if (freelist_head != NULL ) {

		//case1, both blocks before and after are allocated
		//pointers are returning same addresses 
		if ((prevfooter == NULL || prevfooter -> alloc == 0xa) && 
			(nextheader == NULL || nextheader -> alloc == 0xa)) {
			if (LIFO) {
				freecurrentheader -> next = freelist_head;
				freelist_head -> prev = freecurrentheader;
				freelist_head = freecurrentheader;
			} else {
				sf_free_header* cursor = freelist_head;
				int counter = 0;
				while (freecurrentheader > cursor -> next && cursor -> next != 0) {
					cursor = cursor -> next;
					counter++;
				}
				//we're point at the cursor "before" the head list of the list 
				if (counter == 0 && freecurrentheader < cursor) {
					freecurrentheader -> prev = 0;
					freecurrentheader -> next = cursor;
					cursor -> prev = freecurrentheader;
					freelist_head = freecurrentheader;
				} else {
					freecurrentheader -> next = cursor -> next;
					if (cursor -> next != 0)
						cursor -> next -> prev = freecurrentheader;
					cursor -> next = freecurrentheader;
					freecurrentheader -> prev = cursor;
				}
				
			}

		//case 2, block beforehand is free
		} else if ((prevfooter != NULL && prevfooter -> alloc == 0) && 
				   (nextheader == NULL || nextheader -> alloc == 0xa)) {
			//coalesce 
			sf_free_header* prevfreeheader = (sf_free_header*) prevheader;
			prevfreeheader -> header.requested_size += currentheader -> requested_size;
			prevfreeheader -> header.block_size += currentheader -> block_size;
			
			char* prevfootercursor = (char*) prevfooter;
			prevfootercursor += currentheader -> block_size * 16;
			prevfooter = (sf_footer*) prevfootercursor;
			prevfooter -> block_size = prevheader -> block_size;	


			
				//if the prev free header is not the same as the head of the list 
				if(LIFO) {
					//if there are blocks before and after this 
					if (prevfreeheader -> prev != 0 && prevfreeheader -> next != 0) 
						prevfreeheader -> prev -> next = prevfreeheader -> next;
					if(prevfreeheader-> next != 0 && prevfreeheader -> prev != 0)
						prevfreeheader -> next -> prev = prevfreeheader -> prev;

					if (freelist_head != prevfreeheader) {
						freelist_head -> prev = prevfreeheader;
						prevfreeheader -> next = freelist_head;
						freelist_head = prevfreeheader;
						freelist_head -> prev = 0;
					}
				} else {
					if (freecurrentheader -> prev != 0)
						freecurrentheader -> prev -> next = prevfreeheader;
					prevfreeheader -> next = freecurrentheader -> next;
				}

				currentheader -> block_size= 0;
			
		
		//case 3, block after is free
		} else if ((prevfooter == NULL || prevfooter -> alloc == 0xa) && 
				   (nextheader != NULL && nextheader -> alloc == 0)) {
			//coalesce 
			sf_free_header* nextfreeheader = (sf_free_header*) nextheader;
			freecurrentheader -> header.block_size += nextheader -> block_size;
			freecurrentheader -> header.requested_size += nextheader -> requested_size;

			char* currentfootercursor = (char*) currentfooter;
			currentfootercursor += nextheader -> block_size * 16;
			currentfooter = (sf_footer*) currentfootercursor;
			currentfooter -> block_size = freecurrentheader -> header.block_size;

			nextheader -> block_size= 0;
			

			if (LIFO) {
				if (nextfreeheader -> prev != 0 && nextfreeheader -> next != 0)
					nextfreeheader -> prev -> next = nextfreeheader -> next;
				if (nextfreeheader -> next != 0 && nextfreeheader -> prev != 0)
					nextfreeheader -> next -> prev = nextfreeheader -> prev;
				else if (nextfreeheader -> next == 0) {
					if (nextfreeheader -> prev != 0)
						nextfreeheader -> prev -> next = 0;
				}
				if (nextfreeheader == freelist_head) {
					freecurrentheader -> next = freelist_head -> next;
					freecurrentheader -> prev = 0;
					freelist_head = freecurrentheader;
				} else {
					freecurrentheader -> next = freelist_head;

					freecurrentheader -> prev = 0;
					freelist_head = freecurrentheader;
				}
			} else {
				if (nextfreeheader -> prev != 0)
					nextfreeheader -> prev -> next = freecurrentheader;
				if (nextfreeheader -> next != 0)
					nextfreeheader -> next -> prev = freecurrentheader;
				freecurrentheader -> next = nextfreeheader -> next;
				freecurrentheader -> prev = nextfreeheader -> prev;

			} 
			
			
			
		//case 4, both blocks are free
		} else if (prevfooter -> alloc == 0 && nextheader -> alloc == 0) {
			sf_free_header* nextfreeheader = (sf_free_header*) nextheader;
			sf_free_header* prevfreeheader = (sf_free_header*) prevheader;
			prevfreeheader -> header.requested_size += currentheader -> requested_size + 
			nextheader -> requested_size;
			prevfreeheader -> header.block_size += currentheader -> block_size + nextheader -> block_size;

			char* prevfootercursor = (char*) prevfooter;
			prevfootercursor += (currentheader -> block_size + nextheader -> block_size) * 16;
			prevfooter = (sf_footer*) prevfootercursor;
			prevfooter -> block_size = prevfreeheader -> header.block_size;


			//if there are blocks before and after this 
			if (LIFO) {
				if (prevfreeheader -> prev != 0 && prevfreeheader -> next != 0) {
					prevfreeheader -> prev -> next = prevfreeheader -> next;
					prevfreeheader -> next -> prev = prevfreeheader -> prev;
				} 

				if (nextfreeheader -> prev != 0 && nextfreeheader -> next != 0) {
					nextfreeheader -> prev -> next = nextfreeheader -> next;
					nextfreeheader -> next -> prev = nextfreeheader -> prev;
				} else if (nextfreeheader -> next == 0) {
					nextfreeheader -> prev -> next = 0;
				}

				if (freelist_head != prevfreeheader && freelist_head != nextfreeheader) {
					freelist_head -> prev = prevfreeheader;
					prevfreeheader -> next = freelist_head;
					freelist_head = prevfreeheader;
					freelist_head -> prev = 0;
				} else if (freelist_head == prevfreeheader) {
					if (freelist_head -> next != 0){
						freelist_head -> next -> prev = freelist_head;
					if (freelist_head -> next -> next != 0)
						freelist_head -> next = freelist_head -> next -> next;
				}
					freelist_head -> prev = 0;
				} else if (freelist_head == nextfreeheader) {
					prevfreeheader -> next = nextfreeheader -> next;
					freelist_head = prevfreeheader;
					freelist_head -> prev = 0;
				}
			} else {
				prevfreeheader -> next = nextfreeheader -> next;
				if (nextfreeheader -> next != 0) 
					nextfreeheader -> next -> prev = prevfreeheader;
			}

			nextfreeheader -> next = 0;
			nextfreeheader -> prev = 0;
			currentheader -> block_size= 0;
			currentfooter -> block_size= 0;
			nextheader -> block_size= 0;

		} else {
			perror("Something went wrong!");
			errno = ENOMEM;
			exit(EXIT_FAILURE);
		}
	} else {
		freelist_head = freecurrentheader;
		freelist_head -> next = 0;
		freelist_head -> prev = 0;
	}
	
}

void* sf_realloc(void *ptr, size_t size) {
	if (ptr == 0 || size == 0){
		return NULL;
		errno = ENOMEM;
	}

	sf_header* currentheader = (sf_header*) ptr;
	currentheader--;
	void*copy;
	if (currentheader -> block_size > blocksizecalculator(size)) {
		sf_free(ptr);
		copy = sf_malloc(size);

	} else {
		copy = sf_malloc(size);
		memcpy(copy, ptr, size*16);
		sf_free(ptr);
	}
	if (!copy) {
			errno = EINVAL;
			return NULL;
	} else
		return copy;
}

void* sf_calloc(size_t nmemb, size_t size) {
	if (nmemb == 0|| size == 0){
		return NULL;
		errno = ENOMEM;
	}

	 size_t actualsize = nmemb * size; 
	 void *callocptr = sf_malloc(actualsize);
	 if(!callocptr) {
	 	return NULL;
	 	errno = EINVAL;
	 }
	 memset(callocptr, 0, actualsize);
	 return callocptr;
}

sf_header* alloc (sf_header* pointer_header, size_t size) {
	char* cpointer_footer = (char*) pointer_header;
	pointer_header -> alloc = 0xA;
	pointer_header -> requested_size = size;
	pointer_header -> block_size = blocksizecalculator(size);

	cpointer_footer += (pointer_header -> block_size)*16 - 8;
	sf_footer* pointer_footer = (sf_footer*) cpointer_footer;
	pointer_footer -> block_size = pointer_header -> block_size;
	pointer_footer -> alloc = 0xa;
	
	return pointer_header;
}

uint64_t blocksizecalculator(size_t size) {
	if (size < 16) {
			return 2;
		} else {
			if (size % 8 == 0) {
				if ((size / 8) % 2 == 0)
					return((size / 8)+2)/2;
				else
					return((size / 8)+3)/2;
			} else {
				if ((size / 8) % 2 == 0)
					return ((size / 8)+4)/2;
				else
					return ((size / 8)+3)/2;
			}
		}
}

bool valid_address(void* ptr) {
	if (ptr != NULL) {
		sf_header* currentheader = (sf_header*)ptr; 
		currentheader--;
		if ((char*)currentheader > endofmalloc || (void*)currentheader < bottomofheap)
			return false;
		if (currentheader -> alloc == 0 && currentheader -> requested_size == 0 && currentheader -> block_size == 0)
			return false;
		if (currentheader -> alloc != 0 && currentheader -> alloc != 0xa)
			return false;
		if ((int)currentheader -> requested_size < 0)
			return false;
		if ((int)currentheader -> block_size < 0)
			return false;
		if ((long)ptr % 16 != 0)
			return false;
	} else {
		return false;
	}
	return true;
}
