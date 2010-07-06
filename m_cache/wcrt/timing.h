/*! This is a header file of the Chronos timing analyzer. */

/*! Functions to inspect the timing behaviour of tasks */

#ifndef __CHRONOS_TIMING_H
#define __CHRONOS_TIMING_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


char canPreempt( int suspect, int victim );

int canConflict( int suspect, int victim );

int findCriticalPath( chart_t *msc );

char latestTimes( chart_t *msc, int *topoArr, int toposize );

char earliestTimes( chart_t *msc, int *topoArr, int toposize );


#endif
