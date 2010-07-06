/*! This is a header file of the Chronos timing analyzer. */

/*
  Do Level-2 Cache Analysis using Abstract Interpretation
*/

#ifndef __CHRONOS_ANALYSIS_CACHE_L2 
#define __CHRONOS_ANALYSIS_CACHE_L2

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########

static cache_state *
mapLoop_L2(procedure *pro, loop *lp);

static cache_state *
mapFunctionCall_L2(procedure *proc, cache_state *cs);

static void
resetHitMiss_L2(MSC *msc);

static void
resetFunction_L2(procedure *proc);

static void
resetLoop_L2(procedure *proc, loop* lp);

static void
calculateCacheState_L2(cache_line_way_t **must, cache_line_way_t **may,
    cache_line_way_t **persist, int instr_addr);

static cache_state *
copyCacheState_L2(cache_state *cs);

static void
freeAll_L2();

static void
freeAllFunction_L2(procedure *proc);

static void
freeAllLoop_L2(procedure *proc, loop *lp);

static char
isInCache_L2(int addr, cache_line_way_t**must);

static cache_line_way_t **
unionCacheState_L2(cache_line_way_t ** clw_a, cache_line_way_t **clw_b);

/* read basic cache configuration from configFile and then
  set other configuration */
static void
set_cache_basic_L2(char * configFile);

static void
dumpCacheConfig_L2();

static void
calculateMust_L2(cache_line_way_t **must, int instr_addr);

static void
calculateMay_L2(cache_line_way_t **may, int instr_addr);

static void
calculatePersist_L2(cache_line_way_t **persist, int instr_addr);

static void
calculateCacheState_L2(cache_line_way_t **must, cache_line_way_t **may,
    cache_line_way_t **persist, int instr_addr);

/* Needed for checking membership when updating cache
 * state of persistence cache analysis */
static char
isInSet_L2_persist(int addr, cache_line_way_t **set);

static char
isInSet_L2(int addr, cache_line_way_t **set);

/* from way n-> way 0, older->younger */
static cache_line_way_t **
intersectCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

/* from way 0-> way n, younger->older */
static cache_line_way_t **
unionCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

/* from way n +1 -> way 0, older->younger */
static cache_line_way_t **
unionMaxCacheState_L2(cache_line_way_t **clw_a, cache_line_way_t **clw_b);

/* allocate the memory for cache_state */
static cache_state *
allocCacheState_L2();

static void
freeCacheState_L2(cache_state *cs);

static void
freeCacheStateFunction_L2(procedure * proc);

static void
freeCacheStateLoop_L2(procedure *proc, loop *lp);

static void
freeAllFunction_L2(procedure *proc);

static void
freeAllLoop_L2(procedure *proc, loop *lp);

static void
freeAll_L2();

static char
isInCache_L2(int addr, cache_line_way_t**must);

/* For persistence analysis the function would be different */
static cache_line_way_t **
copyCacheSet_persist(cache_line_way_t **cache_set);

static cache_line_way_t **
copyCacheSet(cache_line_way_t **cache_set);

static void
freeCacheSet_L2(cache_line_way_t **cache_set);

static char
isNeverInCache_L2(int addr, cache_line_way_t**may);

static cache_state *
mapLoop_L2(procedure *pro, loop *lp);

static cache_state *
copyCacheState_L2(cache_state *cs);

static cache_state *
mapFunctionCall_L2(procedure *proc, cache_state *cs);

static void
resetFunction_L2(procedure * proc);

static void
resetLoop_L2(procedure * proc, loop * lp);

static void
resetHitMiss_L2(MSC *msc);

/* Do level 2 cache analysis */
static void
cacheAnalysis_L2();

#endif
