#include "allocator.h"
#include "debug_break.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *segment_start;
static size_t segment_size;
static size_t nused;
static char *heap_listp = 0; /* Pointer to first block */

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize) {
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2 * DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize - asize, 0));
  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

/*
 * find_fit - Find a fit for a block with asize bytes
 */
static void *find_fit(size_t asize) {
  /* First-fit search */
  void *bp;

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
      return bp;
    }
  }
  return NULL; /* No fit */
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) { /* Case 1 */
    return bp;
  }

  else if (prev_alloc && !next_alloc) { /* Case 2 */
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  else if (!prev_alloc && next_alloc) { /* Case 3 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else { /* Case 4 */
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  return bp;
}

bool myinit(void *heap_start, size_t heap_size) {
  /* This must be called by a client before making any allocation
   * requests.  The function returns true if initialization was
   * successful, or false otherwise. The myinit function can be
   * called to reset the heap to an empty state. When running
   * against a set of of test scripts, our test harness calls
   * myinit before starting each new script.
   */

  segment_start = heap_start;
  segment_size = (heap_size / DSIZE) * DSIZE;
  nused = 0;
  heap_listp = segment_start;

  PUT(heap_listp, 0);                            /* Alignment padding */
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  // PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
  heap_listp += (2 * WSIZE);

  size_t size = heap_size - 2 * DSIZE;
  char *bp = heap_listp + 2 * WSIZE;
  PUT(HDRP(bp), PACK(size, 0));
  /* Free block header */
  PUT(FTRP(bp), PACK(size, 0));
  /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  nused += 2 * DSIZE;
  return true;
}

void *mymalloc(size_t requested_size) {
  size_t asize; /* Adjusted block size */
  char *bp;
  if (requested_size == 0)
    return NULL;

  /* Adjust block size to include overhead and alignment reqs. */
  if (requested_size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((requested_size + (DSIZE) + (DSIZE - 1)) / DSIZE);

  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    nused += GET_SIZE(HDRP(bp));
    return bp;
  }
  return NULL;
}

void myfree(void *bp) {
  if (bp == 0)
    return;

  size_t size = GET_SIZE(HDRP(bp));
  nused -= size;
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
}

void *myrealloc(void *ptr, size_t size) {
  size_t oldsize;
  void *newptr;

  /* If size == 0 then this is just free, and we return NULL. */
  if (size == 0) {
    myfree(ptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if (ptr == NULL) {
    return mymalloc(size);
  }

  newptr = mymalloc(size);

  /* If realloc() fails the original block is left untouched  */
  if (!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = GET_SIZE(HDRP(ptr));
  if (size < oldsize)
    oldsize = size;
  memcpy(newptr, ptr, oldsize);
  nused += GET_SIZE(HDRP(newptr));
  /* Free the old block. */
  myfree(ptr);

  return newptr;
}

bool validate_heap() {
  /* check your internal structures!
   * Return true if all is ok, or false otherwise.
   * This function is called periodically by the test
   * harness to check the state of the heap allocator.
   * You can also use the breakpoint() function to stop
   * in the debugger - e.g. if (something_is_wrong) breakpoint();
   */
  if (nused > segment_size) {
    printf("Oops! Have used more heap than total available?!\n");
    breakpoint(); // call this function to stop in gdb to poke around
    return false;
  }
  return true;
}
