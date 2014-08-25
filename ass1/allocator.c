//
//  COMP1927 Assignment 1 - Memory Suballocator
//  allocator.c ... implementation
//
//  Created by Liam O'Connor on 18/07/12.
//  Modified by John Shepherd in August 2014
//  Copyright (c) 2012-2014 UNSW. All rights reserved.
//

#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define HEADER_SIZE    sizeof(struct free_list_header)  
#define MAGIC_FREE     0xDEADBEEF
#define MAGIC_ALLOC    0xBEEFDEAD

typedef unsigned char byte;
typedef u_int32_t vlink_t;
typedef u_int32_t vsize_t;
typedef u_int32_t vaddr_t;

typedef struct free_list_header {
   u_int32_t magic;           // ought to contain MAGIC_FREE
   vsize_t size;              // # bytes in this block (including header)
   vlink_t next;              // memory[] index of next free block
   vlink_t prev;              // memory[] index of previous free block
} free_header_t;

// Global data
static byte *memory = NULL;   // pointer to start of suballocator memory
static vaddr_t free_list_ptr; // index in memory[] of first block in free list
static vsize_t memory_size;   // number of bytes malloc'd in memory[]

//Function Prototypes
u_int32_t sizeToN(u_int32_t n);
byte* toPointer(vlink_t index);
vlink_t toIndex(byte* pointer);

//Essential Functions
//Initialise the suballocator, and malloc memory for it
void sal_init(u_int32_t size) {

    //Check if already initialised, do nothing if so
    if (memory != NULL) {
        return;
    }

    //Round size to n
    u_int32_t n = sizeToN(size);

    //set global variables | initialise suballocator
    memory = (byte *)malloc(n);         

    //check if malloc worked properly
    if (memory == NULL){
       fprintf(stderr, "sal_init: insufficient memory");
       abort();
    }

    //set global variables | size and initial location of free list
    free_list_ptr = memory[0]; //index of header of initial block
    memory_size = n;

    //this is dodge???
    //set first free list pointer
    free_header_t *T = (free_header_t *)memory;                                  
    T->magic = MAGIC_FREE;
    T->size = n;
    T->next = 0;                                                                
    T->prev = 0;
}

//Malloc for the program above but using the suballocated region instead
void *sal_malloc(u_int32_t n) {

    //Use the idea of current node to make conceptualising/coding easier
    vlink_t curr = free_list_ptr;

    //Round n to nearest upper power of two, including the header
    n = sizeToN(n + HEADER_SIZE);

/*
    //Check if the allocator is large enough
    if (n > memory_size) {
        return NULL;
    }
*/
    //Scan through list looking for region of size n
    //Makes the while loop work the first time free_list_ptr is passed
    int passCount = 0; 
    //Boolean variable to identify if a suitable region has been found     
    int regionFound = 0;    
    while (regionFound == 0) {
        //Ensure that loop will halt next time it reaches start
        if (curr == free_list_ptr && passCount != 0) {
            fprintf(stderr, "sal_malloc: insufficient memory");
            return NULL;
        }     

        //Print error message if region accessed has already been allocated 
        //(and should therefore have been removed from free list);
        if (toPointer(curr).magic != MAGIC_FREE) {
            fprintf(stderr, "Memory corruption");
            abort();
        }


        //Case if region is sufficiently large    
        } else if ((toPointer(curr).size) >= n) { //this is just going to pick the first region large enough and split it
            regionFound = 1;          //try to go through the whole list and find the smallest one that is large enough
        //Case if region is not large enough
        } else {
            curr = toPointer(curr).next;
        }

        //Increment passCount
        passCount++;
    }

    //Divide segment of memory into smallest possible size
    while (toPointer(curr).size >= 2 * n) { //so it only splits if its more than twice the size, otherwise it will be too small
        curr = memoryDivide(vlink_t curr);
    }

    //Remove region from the free list
    curr = enslaveRegion(curr);

    //Return pointer to the first byte AFTER the region header
    return ((void *)curr + HEADER_SIZE);
}

void sal_free(void *object)
{
   // TODO
}

//Terminate the suballocator - must sal_init to use again
void sal_end(void) {

    //Free all global variables, which makes accessing the (now deleted) suballocator impossible
    free(memory);
    //memory = NULL; //should check if free does this
    free_list_ptr = 0;
    memory_size = 0;

}

void sal_stats(void) {
    //Print the global variables
    printf("sal_stats\n");
    printf("Global Variable 'memory' is: %p\n", memory);
    printf("Global Variable 'free_list_ptr' is: %d\n", free_list_ptr);
    printf("Global Variable 'memory_size' is: %d\n", memory_size);
    
}


//Complementary Functions

//Return usable size from given n value
u_int32_t sizeToN(u_int32_t size) {

    int n = 1;
    //round size to the nearest upper power of two, unless already power of two
    if ((size != 0) && (size & (size - 1)) == 0) {
        n = size;
    } else {
        while (n < size) {
            //This isn't very efficient, but works for time being
            n = 2 * n;
        }
    }

    return n;
}

//Helper Functions
//Converts an index to a pointer
byte* toPointer(vlink_t index) {
    return (memory + index);
}

//Converts a pointer to an index
vlink_t toIndex(byte* pointer) {
    return (pointer - memory);
}