#include "utils.h"

/* Function: roundup
 * -----------------
 * This function rounds up the given number to the given multiple, which
 * must be a power of 2, and returns the result.
 */
size_t roundup(size_t sz, size_t mult) { return (sz + mult - 1) & ~(mult - 1); }