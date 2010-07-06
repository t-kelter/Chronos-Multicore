/*! This is a header file of the Chronos timing analyzer. */

/*! Helper files for dumping internal structures. */

#ifndef __CHRONOS_DUMP_H
#define __CHRONOS_DUMP_H

#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


char* extractTaskName( char *fullname );

/*
 * Returns the name of the task, trimmed if it contains full path.
 */
char* getTaskName( int pidx );

int printTask( int pidx );

int printPE( int peID, int spmCapacity, sched_t *sc );

int printTaskList( chart_t *msc );

int dumpTaskInfo();

int printTask_dep( int pidx );

int dumpTaskInfo_dep();

int printTimes( int idx );

int printMem( int pidx, char verbose );

#endif
