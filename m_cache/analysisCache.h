/*! This is a header file of the Chronos timing analyzer. */

/*!
  Do Level-1 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_H
#define __CHRONOS_ANALYSIS_CACHE_H

#include "header.h"

// ######### Macros #########


// 14-bit tag is selected for the following reason:
// - it is enough: can analyze program up to 16MB for a 1-KB cache
// - in cache.c, a valid bit is needed with tag in some cases, thus the valid
//   bit and the tag can be accommodated in a short int
#define MAX_TAG_BITS        14

#define CACHE_LINE(addr)    ((addr) & cache.l_msk)
#define SET(addr)       (((addr) & cache.s_msk) >> cache.lsb)
#define TAG(addr)       (((addr) & cache.t_msk) >> cache.s_lb)
#define TAGSET(addr)        (((addr) & cache.t_s_msk) >> cache.lsb)
#define TAGSET2(tag, set)   (((tag) << cache.nsb) | (set))
#define LSB_OFF(addr)       ((addr) >> cache.lsb)
#define LSB_ON(addr)        ((addr) << cache.lsb)

#define CACHE_LINE_L2(addr)    ((addr) & cache_L2.l_msk)
#define SET_L2(addr)       (((addr) & cache_L2.s_msk) >> cache_L2.lsb)
#define TAG_L2(addr)       (((addr) & cache_L2.t_msk) >> cache_L2.s_lb)
#define TAGSET_L2(addr)        (((addr) & cache_L2.t_s_msk) >> cache_L2.lsb)
#define TAGSET2_L2(tag, set)   (((tag) << cache_L2.nsb) | (set))
#define LSB_OFF_L2(addr)       ((addr) >> cache_L2.lsb)
#define LSB_ON_L2(addr)        ((addr) << cache_L2.lsb)

// clear the LSB bits (all LSB bits are set to 0 whatever they are)
#define CLEAR_LSB(addr)     (((addr) >> cache.lsb) << cache.lsb)
#define CLEAR_LSB_L2(addr)     (((addr) >> cache_L2.lsb) << cache_L2.lsb)

#define MBLK_ID(sa, addr)   (TAGSET(addr) - TAGSET(sa))

#define MAX_CACHE_SETS      1024

#define IC_HIT          1
#define IC_MISS         10
//#define IC_UNCLEAR      2

#define IC_HIT_L2          10
#define IC_MISS_L2        100

#define MC_INC_SIZE     8


#define ALWAYS_HIT 1
#define ALWAYS_MISS 2
#define FIRST_MISS 3
#define UNKNOW 4
#define HIT_UPPER 5


// ######### Datatype declarations  ###########



// ######### Function declarations  ###########

// TODO: Move this function out of here into some util file
int
logBase2(int n);

/* read basic cache configuration from configFile and then 
   set other configuration */
void
set_cache_basic(char * configFile);

void
dumpCacheConfig();

void
constructAll(task_t *task);

/* Do level one cache analysis */
void
cacheAnalysis();

char 
isInWay(int entry, int *entries, int num_entry);

void
freeCacheSet(cache_line_way_t **cache_set);

void
freeAllCacheState();

#endif
