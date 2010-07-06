/*! This is a header file of the Chronos timing analyzer. */

/*!
  Do Level-1 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_H
#define __CHRONOS_ANALYSIS_CACHE_H

#include "header.h"

// ######### Macros #########



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
