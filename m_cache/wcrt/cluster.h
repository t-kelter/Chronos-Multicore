/*! This is a header file of the Chronos timing analyzer. */

/*! This file holds helper functions for task clustering */

#ifndef __CHRONOS_CLUSTER_H
#define __CHRONOS_CLUSTER_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/*
 * Do clustering by latestTimes.
 */
char doClustering( sched_t *sc, chart_t *msc );


#endif
