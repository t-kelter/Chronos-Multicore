/*! This is a header file of the Chronos timing analyzer. */

/*!
  Do Level-1 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_L1_H
#define __CHRONOS_ANALYSIS_CACHE_L1_H

#include "header.h"

// ######### Macros #########


#define CACHE_LINE(addr)    ((addr) & cache.l_msk)
#define SET(addr)       (((addr) & cache.s_msk) >> cache.lsb)
#define TAG(addr)       (((addr) & cache.t_msk) >> cache.s_lb)
#define TAGSET(addr)        (((addr) & cache.t_s_msk) >> cache.lsb)
#define TAGSET2(tag, set)   (((tag) << cache.nsb) | (set))
#define LSB_OFF(addr)       ((addr) >> cache.lsb)
#define LSB_ON(addr)        ((addr) << cache.lsb)

// clear the LSB bits (all LSB bits are set to 0 whatever they are)
#define CLEAR_LSB(addr)     (((addr) >> cache.lsb) << cache.lsb)

#define MBLK_ID(sa, addr)   (TAGSET(addr) - TAGSET(sa))

#define MAX_CACHE_SETS      1024

#define MC_INC_SIZE     8


// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


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

void
freeCacheSet(cache_line_way_t **cache_set);

void
freeAllCacheState();

#endif
