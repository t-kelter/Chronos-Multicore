/*! This is a header file of the Chronos timing analyzer. */

/*
  Update Level-2 Cache State due to conflicting with other cores
*/

#ifndef __CHRONOS_UPDATE_CACHE_L2_H
#define __CHRONOS_UPDATE_CACHE_L2_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


void
updateCacheState(MSC *msc);

static void
updateLoop(MSC *msc, int index, procedure *proc, loop *lp);

static void
updateFunctionCall(MSC *msc, int index, procedure* proc);

static int
conflictStatistics(MSC *msc, int index, int set_no, int current_addr);


#endif
