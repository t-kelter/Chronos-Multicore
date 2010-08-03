/*! This is a header file of the Chronos timing analyzer. */

/*
  Do Level-2 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_L2 
#define __CHRONOS_ANALYSIS_CACHE_L2

// ######### Macros #########


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
