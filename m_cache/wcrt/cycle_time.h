/*! This is a header file of the Chronos timing analyzer. */

/*! This file holds functions for measuring the elapsed time. */

#ifndef __CHRONOS_CYCLE_TIME_H
#define __CHRONOS_CYCLE_TIME_H

// ######### Macros #########



// ######### Datatype declarations  ###########



// ######### Function declarations  ###########

/* 
 * Time-keeping:
 * Change CYCLES_PER_MSEC according to CPU speed where this is run.
 * e.g. 3 GHz = 3E+9 cyc/sec = 3E+6 cyc/millisec
 * CLOCKS_PER_SEC is defined in <time.h>
 */

#define CYCLES_PER_MSEC 3000000
#define STARTTIME cycle_time(0)
#define STOPTIME double t = cycle_time(1); printf( "=== %f ms\n", t/CYCLES_PER_MSEC ); fflush( stdout )


/* This functions starts and ends the time measurement.
 *  
 * start_end == 0: start counting the time
 * start_end == 1: end counting the time and return the time elapsed (in cycles)
 */
double cycle_time(int start_end);


#endif
