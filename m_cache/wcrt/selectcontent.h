/*! This is a header file of the Chronos timing analyzer. */

/*! Functions for allocating objects from a task to a given scratchpad portion. */

#ifndef __CHRONOS_SELECTCONTENT_H
#define __CHRONOS_SELECTCONTENT_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Use for code SPM.
 */
int doAllocationILP( chart_t *msc, overlay_t *ox, int capacity );

/*
 * Use for data SPM.
 */
int doAllocationKnapsack( overlay_t *ox, int capacity );

int doAllocation( chart_t *msc, overlay_t *ox, int capacity );


#endif
