/*! This is a header file of the Chronos timing analyzer. */

/*! Functins for managing the bus schedule */

#ifndef __CHRONOS_BUS_SCHEDULE_H
#define __CHRONOS_BUS_SCHEDULE_H

#include <stdio.h>
#include "header.h"

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########


/* Improper exit with error message */
void prerr(char* msg);

/* return the global TDMA bus schedule set previously */
sched_p getSchedule();

void setSchedule(const char* sched_file);

/* Gets the schedule for the core with index 'core_index' at time 'time'
 * in the current global TDMA schedule. */
core_sched_p getCoreSchedule( uint core_index, ull time );

#endif
