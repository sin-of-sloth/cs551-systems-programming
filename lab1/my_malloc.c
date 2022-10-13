/********************************************************************
*   THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING     *
*   A TUTOR OR CODE WRITTEN BY OTHER STUDENTS                       *
*                               - ARJUN LAL                         *
********************************************************************/


#include<stddef.h>
#include<unistd.h>
#include "my_malloc.h"

// default sbrk size
#define SBRK_DEFAULT_SIZE 8192
// size of a freelistnode
#define FREELISTNODE_SIZE sizeof(FreeListNode)
// minimum size of a chunk that can be in freelist
#define MIN_CHUNK_SIZE 16
// fragmentation required to make chunk size a multiple of 8
#define FRAGMENT(size) ((8 - (size % 8)) % 8)


FreeListNode freelistbase = NULL;


//free_list_begin(): returns pointer to first chunk in free list
FreeListNode free_list_begin(void) {
    return freelistbase;
}


// returns required chunk size accounting for chunk header
// and internal fragmentation
size_t required_chunk_size(size_t size) {
    return CHUNKHEADERSIZE + size + FRAGMENT(size);
}


// returns size to be requested from sbrk
int req_sbrk_size(size_t size) {
    int default_size = SBRK_DEFAULT_SIZE;
    // if default sbrk size (8192) can fit the required size
    // and have space left for the smallest chunk possible
    // (16 - 8 for header, another 8 considering internal fragmentation
    // for lowest request size 1), return default size
    if(default_size >= size + 16) {
        return default_size;
    }
    // if requested size is exactly 8192, or if it's bigger,
    // return requested size
    else if(default_size <= size) {
        return (int) size;
    }
    else {
        // means SBRK_SIZE > size, but SBRK_SIZE - size < 16
        // so after assigning chunk of size "size", the remaining
        // won't be of minimum chunk size
        // can only happen when "size" is 8184 ("size" is a multiple of 8)
        return default_size + 8;
    }
}


// if there's a node that is usable for the size,
// remove it from freelist, insert the remaining chunk
// in free list, and return the needed chunk
void * find_node_in_freelist(size_t size) {

    // if freelist is empty, return NULL
    if(freelistbase == NULL) {
        return NULL;
    }
    void * usable_chunk = NULL;

    // if the first node is more than big enough, get required sized chunk
    // and insert the rest back in freelist
    if(freelistbase->size >= size + MIN_CHUNK_SIZE) {
        usable_chunk = (void *) freelistbase;
        FreeListNode big_fnode = (void *) freelistbase + size;
        big_fnode->size = freelistbase->size - size;
        big_fnode->flink = freelistbase->flink;
        freelistbase = big_fnode;
    }
    // else if the first node is exactly enough, return that chunk -
    // there will be nothing to add back to freelistbase
    else if(freelistbase->size == size) {
        usable_chunk = (void *) freelistbase;
        freelistbase = freelistbase->flink;
    }
    // else go through the list to find a usable node
    else {
        FreeListNode fnode = freelistbase;

        while(fnode->flink != NULL) {
            // if the next node is big enough, that would be the usable chunk
            // insert remaining back into freelist
            if(fnode->flink->size >= size + MIN_CHUNK_SIZE) {
                usable_chunk = (void *) fnode->flink;
                FreeListNode next_fnode = (void *) fnode->flink + size;
                next_fnode->size = fnode->flink->size - size;
                next_fnode->flink = fnode->flink->flink;
                fnode->flink = next_fnode;
                break;
            }
            // if the next node is exactly enough, remove it from the freelist
            // and return it - there will be nothing to add back to freelist
            if(fnode->flink->size == size) {
                usable_chunk = (void *) fnode->flink;
                fnode->flink = fnode->flink->flink;
                break;
            }
            // if nothing's found, move to next node
            fnode = fnode->flink;
        }
    }

    return usable_chunk;
}


// inserts a node into the freelist
void insert_node_in_freelist(FreeListNode fnode) {
    FreeListNode fptr = freelistbase;
    // if the freelist is empty, make the node the freelist
    if(freelistbase == NULL) {
        freelistbase = fnode;
    }
    // if the node comes before the first element of freelist,
    // make the node the start
    else if(fnode < fptr) {
        fnode->flink = fptr;
        freelistbase = fnode;
    }
    else {
        while(fptr->flink != NULL) {
            if(fnode < fptr->flink) {
                break;
            }
            fptr = fptr->flink;
        }
        // reach here if we find the correct place,
        // or if we're at the end of the list
        fnode->flink = fptr->flink;
        fptr->flink = fnode;
    }
}


//my_malloc: returns a pointer to a chunk of heap allocated memory
void *my_malloc(size_t size) {
    MyErrorNo my_errno = MYNOERROR;
    if(size < 1) {
        my_errno = MYENOMEM;
        return NULL;
    }
    FreeListNode fnode = NULL;
    size_t req_chunk_size = required_chunk_size(size);

    int *chunk_size, *magic_number;
    void *ret_ptr = find_node_in_freelist(req_chunk_size);

    // if unable to find a usable chunk in freelist, we need to sbrk from heap
    if(ret_ptr == NULL) {
        int sbrk_size = req_sbrk_size(req_chunk_size);
    // printf("sbrk_amt: %d\tcurr_heap_end: %p\t", sbrk_size, sbrk(0));
        fnode = (FreeListNode) sbrk(sbrk_size);
        fnode->flink = NULL;
        fnode->size = sbrk_size;
        insert_node_in_freelist(fnode);

        // we've done sbrk again and inserted a suitable chunk in freelist -
        // try getting a suitable chunk again
        ret_ptr = find_node_in_freelist(req_chunk_size);
    }
    chunk_size = ret_ptr;
    *chunk_size = (int) req_chunk_size;
    magic_number = (void *) ret_ptr + 4;
    *magic_number = (int) ~(req_chunk_size);
    return (void *) ret_ptr + 8;
}


// my_free: reclaims the previously allocated chunk referenced by ptr
void my_free(void *ptr) {
    MyErrorNo my_errno = MYNOERROR;
    if(ptr == NULL) {
        my_errno = MYBADFREEPTR;
        return;
    }

    int chunk_size = *((int *) (ptr - 8));
    int magic_number = *((int *) (ptr - 4));
    if(magic_number != ~chunk_size) {
        my_errno = MYBADFREEPTR;
        return;
    }
    FreeListNode fnode = ptr - 8;
    fnode->flink = NULL;
    fnode->size = chunk_size;
    insert_node_in_freelist(fnode);
}


// coalesce_free_list(): merge adjacent chunks on the free list
void coalesce_free_list(void) {
    if(freelistbase == NULL) {
        return;
    }
    FreeListNode fnode = freelistbase;
    while(fnode->flink != NULL) {
        void *fnode_end = (void *) fnode + fnode->size;
        if(fnode_end == fnode->flink) {
            fnode->size += fnode->flink->size;
            fnode->flink = fnode->flink->flink;
            continue;
        }
        fnode = fnode->flink;
    }
}
