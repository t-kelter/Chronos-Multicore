/*! This is a header file of the Chronos timing analyzer. */

/*! Functions for computing the interference between tasks */

#ifndef __CHRONOS_SLACKS_H
#define __CHRONOS_SLACKS_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


uint get_msc_id(chart_t* msc, uint task_id);

int resetInterference( chart_t *msc );

void generateWeiConflict(chart_t *msc);

int writeWeiConflict();

/* Analyze the interference between tasks within msc */
void setInterference( chart_t *msc );

int writeInterference();

void dumpInterference( chart_t *msc );

/*
 * Returns 1 if pred comes before succ in timeTopoList.
 */
char comesBefore( int pred, int succ, chart_t *msc );

char latestTimes_slack( chart_t *msc, int *topoArr, int toposize );

char earliestTimes_slack( chart_t *msc, int *topoArr, int toposize );

char doTiming( chart_t *msc, char slack );

int getAllocatedSize( task_t *ts );

int insertSlacks( chart_t *msc );


#endif
