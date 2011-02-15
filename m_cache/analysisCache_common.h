/*! This is a header file of the Chronos timing analyzer. */

/*!
  Cache Analysis helper functions
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_COMMON_H
#define __CHRONOS_ANALYSIS_CACHE_COMMON_H

#include "header.h"

// ######### Macros #########


// 14-bit tag is selected for the following reason:
// - it is enough: can analyze program up to 16MB for a 1-KB cache
// - in cache.c, a valid bit is needed with tag in some cases, thus the valid
//   bit and the tag can be accommodated in a short int
#define MAX_TAG_BITS        14

#define IC_HIT          1
#define IC_MISS         10
#define IC_HIT_L2          10
#define IC_MISS_L2        100

#define ALWAYS_HIT 1
#define ALWAYS_MISS 2
#define FIRST_MISS 3
#define UNKNOW 4
#define HIT_UPPER 5


// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int
logBase2(int n);

char
isInWay(int entry, int *entries, int num_entry);


#endif
