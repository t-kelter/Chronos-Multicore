/*! This is a header file of the Chronos timing analyzer. */

/*! This file contains central functions for generating scratchpad allocations. */

#ifndef __CHRONOS_ALLOC_H
#define __CHRONOS_ALLOC_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Determine lower bound of a task's runtime by letting it have the whole SPM capacity.
 */
int firstAlloc();

int resetAllocation( chart_t *msc );

  /*
 * Clustering + graph coloring to determine time-sharing tasks.
 * SPM space is then partitioned to the number of colors.
 */
int updateAllocationGC( sched_t *sc, chart_t *msc, int spmCapacity );

/*
 * Simple clustering--partitioning method.
 */
int updateAllocationIC( sched_t *sc, chart_t *msc, int spmCapacity );

/*
 * Simple static knapsack allocation method.
 */
int updateAllocationPK( sched_t *sc, chart_t *msc, int spmCapacity );

char updateAllocation( int peID, chart_t *msc );

int initAlloc( chart_t *msc );

int timingAllocMSC( chart_t *msc );

int timingAllocMSC_slackCritical( chart_t *msc );


#endif
