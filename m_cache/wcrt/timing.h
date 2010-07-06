/*! This is a header file of the Chronos timing analyzer. */

/*! Functions to inspect the timing behaviour of tasks */

#ifndef __CHRONOS_TIMING_H
#define __CHRONOS_TIMING_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


int timingEstimateMSC( chart_t *msc );

char canPreempt( int suspect, int victim );

int canConflict( int suspect, int victim );

int findCriticalPath( chart_t *msc );

time_t gxCalcHi( chart_t *msc, int idx, time_t x );
time_t gxCalcLo( chart_t *msc, int idx, time_t x );

time_t fixedPointCalcHi( chart_t *msc, int idx );
time_t fixedPointCalcLo( chart_t *msc, int idx );

char latestTimes( chart_t *msc, int *topoArr, int toposize );
char earliestTimes( chart_t *msc, int *topoArr, int toposize );

/*
 * WCRT of the whole application is defined as:
 *   max{t}(latestFin(t)) - min{t}(earliestReq(t))
 */
time_t computeWCRT( chart_t *msc );

int timingEstimateMSC( chart_t *msc );

#endif
