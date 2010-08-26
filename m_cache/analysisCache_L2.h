/*! This is a header file of the Chronos timing analyzer. */

/*
  Do Level-2 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_L2_H
#define __CHRONOS_ANALYSIS_CACHE_L2_H

#include "header.h"

// ######### Macros #########


#define CACHE_LINE_L2(addr)    ((addr) & cache_L2.l_msk)
#define SET_L2(addr)       (((addr) & cache_L2.s_msk) >> cache_L2.lsb)
#define TAG_L2(addr)       (((addr) & cache_L2.t_msk) >> cache_L2.s_lb)
#define TAGSET_L2(addr)        (((addr) & cache_L2.t_s_msk) >> cache_L2.lsb)
#define TAGSET2_L2(tag, set)   (((tag) << cache_L2.nsb) | (set))
#define LSB_OFF_L2(addr)       ((addr) >> cache_L2.lsb)
#define LSB_ON_L2(addr)        ((addr) << cache_L2.lsb)

// clear the LSB bits (all LSB bits are set to 0 whatever they are)
#define CLEAR_LSB_L2(addr)     (((addr) >> cache_L2.lsb) << cache_L2.lsb)

#define MISS 0
#define HIT 1

#define NOT_USED 0
#define USED   1


// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/* read basic cache configuration from configFile and then
  set other configuration */
void
set_cache_basic_L2(char * configFile);

void
dumpCacheConfig_L2();

void
freeAll_L2();

void
resetHitMiss_L2(MSC *msc);

/* Do level 2 cache analysis */
void
cacheAnalysis_L2();

#endif
