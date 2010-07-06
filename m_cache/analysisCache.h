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

static void
freeCacheSet(cache_line_way_t **cache_set);

static procedure*
constructFunctionCall(procedure *pro, task_t *task);

static block* 
copyBlock(block * bb);

static loop* 
copyLoop(procedure *proc, procedure* copy_proc, loop* lp);

static procedure* 
copyFunction(procedure *proc);

static void
freeAllCacheState();

static void
freeAllFunction(procedure *proc);

static void
freeAllLoop(procedure *proc, loop *lp);

static cache_state*
mapLoop(procedure *proc, loop *lp);

static cache_state *
mapFunctionCall(procedure *proc, cache_state *cs);

static void
calculateCacheState(cache_line_way_t **must, cache_line_way_t **may, cache_line_way_t **persist, int instr_addr);

static cache_state *
copyCacheState(cache_state *cs);

static void dump_cache_line(cache_line_way_t *clw_a);

static void
freeCacheState(cache_state *cs);

static void
freeCacheStateLoop(procedure *proc, loop *lp);

static void
freeCacheStateFunction(procedure *proc);

static char 
isInWay(int entry, int *entries, int num_entry);

static char
isNeverInCache(int addr, cache_line_way_t**may);

static char
isInCache(int addr, cache_line_way_t**must);

static cache_line_way_t **
unionCacheState(cache_line_way_t ** clw_a, cache_line_way_t **clw_b);

static cache_line_way_t **
intersectCacheState(cache_line_way_t ** clw_a, cache_line_way_t **clw_b);

static cache_line_way_t **
unionMaxCacheState(cache_line_way_t ** clw_a, cache_line_way_t **clw_b);

static int
logBase2(int n);

/* read basic cache configuration from configFile and then 
   set other configuration */
static void
set_cache_basic(char * configFile);

static void
dumpCacheConfig();

static block* 
copyBlock(block *bb);

static loop* 
copyLoop(procedure *proc, procedure* copy_proc, loop *lp);

static procedure* 
copyFunction(procedure* proc);

static procedure*
constructFunctionCall(procedure *pro, task_t *task);

static void
constructAll(task_t *task);

static void
calculateMust(cache_line_way_t **must, int instr_addr);

static void
calculateMay(cache_line_way_t **may, int instr_addr);

static void
calculatePersist(cache_line_way_t **persist, int instr_addr);

static void
calculateCacheState(cache_line_way_t **must, cache_line_way_t **may,
    cache_line_way_t **persist, int instr_addr);

/* allocate the memory for cache_state */
static cache_state *
allocCacheState();

/* For checking when updating the cache state during 
 * persistence analysis */
static char
isInSet_persist(int addr, cache_line_way_t **set);

static char
isInSet(int addr, cache_line_way_t **set);

static char
isInCache(int addr, cache_line_way_t**must);

static char
isNeverInCache(int addr, cache_line_way_t**may);

static cache_state *
copyCacheState(cache_state *cs);

static cache_state *
mapLoop(procedure *proc, loop *lp);

static cache_state *
mapFunctionCall(procedure *proc, cache_state *cs);

//do level one cache analysis
static void
cacheAnalysis();

static char 
isInWay(int entry, int *entries, int num_entry);

// from way 0-> way n, younger->older
static cache_line_way_t **
unionCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

// from way n +1 -> way 0, older->younger
static cache_line_way_t **
unionMaxCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

static void
freeCacheSet(cache_line_way_t **cache_set);

static void
freeCacheState(cache_state *cs);

static void
freeCacheStateFunction(procedure * proc);

static void
freeCacheStateLoop(procedure *proc, loop *lp);

static void
freeAllFunction(procedure *proc);

static void
freeAllLoop(procedure *proc, loop *lp);

static void
freeAllCacheState();

/* from way n-> way 0, older->younger */
static cache_line_way_t **
intersectCacheState(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

static void
dump_cache_line(cache_line_way_t *clw_a);

static char test_cs_op();

#endif
