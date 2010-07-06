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

/* Prints read TDMA bus schedule */
void print_core_specific_data(core_sched_p* head_core, int ncore, FILE* fp);

void print_TDMA_sched();

/* Find proper segment given a list of segments and a starting time */
segment_p find_segment(segment_p* head_seg, int nsegs, ull start_time);

/* return the global TDMA bus schedule set previously */
sched_p getSchedule();

void setSchedule(const char* sched_file);


#endif
