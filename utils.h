/* File: allocator.h
 * -----------------
 * Interface file for the custom heap allocator.
 */
#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE                                                                  \
  4 /* Word and header/footer size (bytes) */ // line:vm:mm:beginconst
#define DSIZE 8                               /* Double word size (bytes) */
#define CHUNKSIZE                                                              \
  (1 << 12) /* Extend heap by this amount (bytes) */ // line:vm:mm:endconst

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc)) // line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))              // line:vm:mm:get
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1) // line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)                        // line:vm:mm:hdrp
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)                                                          \
  ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) // line:vm:mm:nextblkp
#define PREV_BLKP(bp)                                                          \
  ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE))) // line:vm:mm:prevblkp
/* $end mallocmacros */

/* Function: roundup
 * -----------------
 * This function rounds up the given number to the given multiple, which
 * must be a power of 2, and returns the result.
 */
size_t roundup(size_t, size_t);

#endif